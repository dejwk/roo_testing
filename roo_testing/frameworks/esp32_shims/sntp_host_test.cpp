#include "roo_testing/frameworks/esp32_shims/sntp_host.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>

#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

namespace {
namespace host = roo_testing::esp32::sntp;
std::atomic<int> calls{0};
std::atomic<int64_t> received{0};
std::atomic<bool> published{false};
std::atomic<bool> in_isr{true};
std::atomic<TaskHandle_t> callback_task{nullptr};
enum class Action { kNone, kStop, kRestartOnce };
Action action = Action::kNone;

void Notification(struct timeval* value) {
  const int64_t micros = value->tv_sec * 1000000LL + value->tv_usec;
  int64_t now;
  received = micros;
  published = roo_testing::system::GetUnixTimeMicros(now) && now >= micros;
  in_isr = xPortInIsrContext();
  callback_task = xTaskGetCurrentTaskHandle();
  if (action == Action::kStop) esp_sntp_stop();
  if (action == Action::kRestartOnce) {
    if (calls.load() == 0) {
      EXPECT_TRUE(esp_sntp_restart());
    } else {
      esp_sntp_stop();
    }
  }
  ++calls;
}

// Waits for task delivery without advancing manual uptime or assuming inline
// work.
void WaitForCalls(int expected) {
  for (int i = 0; i < 1000 && calls.load() < expected; ++i) vTaskDelay(1);
  ASSERT_GE(calls.load(), expected);
}

class SntpTest : public testing::Test {
 protected:
  void SetUp() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
    calls = 0;
    received = 0;
    published = false;
    in_isr = true;
    callback_task = nullptr;
    action = Action::kNone;
    host::ServerBehavior behavior;
    behavior.time_source = host::TimeSource::kReference;
    behavior.reference_unix_us = 1700000000123456LL;
    behavior.response_delay_us = 5000;
    ASSERT_EQ(ESP_OK, host::ConfigureServer(behavior));
    esp_sntp_set_time_sync_notification_cb(Notification);
    esp_sntp_setservername(0, "test.invalid");
  }
  void TearDown() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
  }
};

// Verifies delayed delivery publishes UTC and runs on a different non-ISR task.
TEST_F(SntpTest, ReferenceResponseAndStatus) {
  const TaskHandle_t caller = xTaskGetCurrentTaskHandle();
  EXPECT_EQ(SNTP_SYNC_STATUS_RESET, esp_sntp_get_sync_status());
  esp_sntp_init();
  if (!system_time_is_auto_sync_enabled()) {
    system_time_delay_micros(4999);
    EXPECT_EQ(0, calls.load());
    system_time_delay_micros(1);
  } else {
    system_time_delay_micros(5000);
  }
  WaitForCalls(1);
  EXPECT_TRUE(published.load());
  EXPECT_FALSE(in_isr.load());
  EXPECT_NE(caller, callback_task.load());
  EXPECT_GE(received.load(), 1700000000123456LL + 5000);
  EXPECT_EQ(SNTP_SYNC_STATUS_COMPLETED, esp_sntp_get_sync_status());
  EXPECT_EQ(SNTP_SYNC_STATUS_RESET, esp_sntp_get_sync_status());
  EXPECT_EQ(1, esp_sntp_getreachability(0));
}

// Verifies a silent server never initializes device UTC or emits a
// notification.
TEST_F(SntpTest, NoResponseAndAtomicConfigurationFailure) {
  host::ServerBehavior behavior;
  behavior.responds = false;
  behavior.response_delay_us = 1000;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(behavior));
  esp_sntp_init();
  EXPECT_EQ(ESP_ERR_INVALID_STATE,
            host::ConfigureServer(host::ServerBehavior{}));
  system_time_delay_micros(2000);
  vTaskDelay(1);
  EXPECT_EQ(0, calls.load());
  EXPECT_EQ(SNTP_SYNC_STATUS_RESET, esp_sntp_get_sync_status());
  int64_t utc;
  ASSERT_TRUE(roo_testing::system::GetUnixTimeMicros(utc));
  EXPECT_LT(utc, 1000000000000LL);
}

// Verifies stopped pending work is harmless and an explicit restart runs anew.
TEST_F(SntpTest, StopPendingThenRestart) {
  esp_sntp_init();
  esp_sntp_stop();
  system_time_delay_micros(10000);
  vTaskDelay(1);
  EXPECT_EQ(0, calls.load());
  EXPECT_FALSE(esp_sntp_restart());
  esp_sntp_init();
  system_time_delay_micros(5000);
  WaitForCalls(1);
  const int64_t synced = received.load();
  host::Reset();
  int64_t utc;
  ASSERT_TRUE(roo_testing::system::GetUnixTimeMicros(utc));
  EXPECT_GE(utc, synced);
}

// Verifies callbacks may restart or stop without re-entering a held client
// lock.
TEST_F(SntpTest, CallbackReentry) {
  action = Action::kRestartOnce;
  esp_sntp_init();
  system_time_delay_micros(5000);
  WaitForCalls(1);
  system_time_delay_micros(5000);
  WaitForCalls(2);
  EXPECT_FALSE(esp_sntp_enabled());
}

// Verifies copied names, interval clamping, and invalid scenario preservation.
TEST_F(SntpTest, ConfigurationAndValidation) {
  std::string name = "copied.invalid";
  esp_sntp_setservername(1, name.c_str());
  name.assign("changed");
  EXPECT_STREQ("copied.invalid", esp_sntp_getservername(1));
  esp_sntp_setservername(1, nullptr);
  EXPECT_EQ(nullptr, esp_sntp_getservername(1));
  esp_sntp_set_sync_interval(1);
  EXPECT_EQ(15000u, esp_sntp_get_sync_interval());
  host::ServerBehavior bad;
  bad.response_delay_us = 0;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, host::ConfigureServer(bad));
  bad.response_delay_us = 1;
  bad.reference_unix_us = INT64_MAX;
  EXPECT_EQ(ESP_ERR_INVALID_ARG, host::ConfigureServer(bad));
}

// Verifies a true manual-time jump coalesces overdue polls instead of
// replaying.
TEST_F(SntpTest, CoalescesLagJump) {
  if (system_time_is_auto_sync_enabled()) GTEST_SKIP();
  esp_sntp_set_sync_interval(15000);
  esp_sntp_init();
  system_time_lag_ns(3600000000000ULL);
  ProcessSystemTimeAlarms();
  WaitForCalls(1);
  vTaskDelay(1);
  EXPECT_EQ(1, calls.load());
}
}  // namespace
