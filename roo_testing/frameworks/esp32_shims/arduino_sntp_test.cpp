#include <Arduino.h>
#include <esp_sntp.h>
#include <gtest/gtest.h>
#include <sys/time.h>

#include <atomic>
#include <cstdlib>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

namespace {
namespace host = roo_testing::esp32::sntp;
std::atomic<int> calls{0};
std::atomic<bool> clock_published{false};
void Notify(struct timeval* value) {
  struct timeval now;
  clock_published =
      gettimeofday(&now, nullptr) == 0 && now.tv_sec >= value->tv_sec;
  ++calls;
}
class ArduinoSntp : public testing::Test {
 protected:
  void SetUp() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
    const char* tz = getenv("TZ");
    had_tz_ = tz != nullptr;
    old_tz_ = had_tz_ ? tz : "";
    calls = 0;
    clock_published = false;
    esp_sntp_set_time_sync_notification_cb(Notify);
  }
  void TearDown() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
    if (had_tz_)
      setenv("TZ", old_tz_.c_str(), 1);
    else
      unsetenv("TZ");
    tzset();
  }
  bool had_tz_ = false;
  std::string old_tz_;
};

// Verifies a cold clock remains epoch-era until an actual simulated reply
// arrives.
TEST_F(ArduinoSntp, ColdBootThenReferenceSync) {
  struct tm calendar;
  EXPECT_FALSE(getLocalTime(&calendar, 1));
  EXPECT_LT(time(nullptr), 1000000000);
  host::ServerBehavior server;
  server.time_source = host::TimeSource::kReference;
  server.reference_unix_us = 1700000000000000;
  server.response_delay_us = 100000;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(server));
  configTime(3600, 0, "virtual.invalid", "backup.invalid");
  EXPECT_EQ(0, calls.load());
  EXPECT_STREQ("backup.invalid", esp_sntp_getservername(1));
  system_time_delay_micros(100000);
  for (int i = 0; i < 1000 && calls.load() == 0; ++i) vTaskDelay(1);
  ASSERT_EQ(1, calls.load());
  EXPECT_TRUE(clock_published.load());
  ASSERT_TRUE(getLocalTime(&calendar, 1));
  const time_t now = time(nullptr);
  struct tm utc;
  gmtime_r(&now, &utc);
  EXPECT_EQ((utc.tm_hour + 1) % 24, calendar.tm_hour);
}

// Verifies a silent server cannot make a cold clock appear synchronized.
TEST_F(ArduinoSntp, SilentServerTimesOut) {
  host::ServerBehavior server;
  server.responds = false;
  server.response_delay_us = 1000;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(server));
  configTzTime("UTC0", "silent.invalid");
  struct tm calendar;
  EXPECT_FALSE(getLocalTime(&calendar, 20));
  EXPECT_EQ(0, calls.load());
  EXPECT_EQ(SNTP_SYNC_STATUS_RESET, esp_sntp_get_sync_status());
}

// Verifies a manually set clock passes the year heuristic without an SNTP
// event.
TEST_F(ArduinoSntp, ManualClockIsNotSntpCompletion) {
  const struct timeval value = {1700000000, 123};
  ASSERT_EQ(0, settimeofday(&value, nullptr));
  configTzTime("UTC0", nullptr);
  struct tm calendar;
  EXPECT_TRUE(getLocalTime(&calendar, 0));
  EXPECT_EQ(0, calls.load());
  EXPECT_EQ(SNTP_SYNC_STATUS_RESET, esp_sntp_get_sync_status());
  esp_sntp_stop();
  EXPECT_GE(time(nullptr), value.tv_sec);
}
}  // namespace
