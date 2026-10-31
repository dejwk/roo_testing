#include <gtest/gtest.h>

#include <atomic>

#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

namespace {
namespace host = roo_testing::esp32::sntp;
std::atomic<int> notifications{0};
std::atomic<int> events{0};
std::atomic<int64_t> notified_us{0};
std::atomic<int64_t> event_us{0};
std::atomic<bool> deinit_on_notify{false};

void Notification(struct timeval* tv) {
  notified_us = tv->tv_sec * 1000000LL + tv->tv_usec;
  if (deinit_on_notify.load()) esp_netif_sntp_deinit();
  ++notifications;
}
void SyncEvent(void*, esp_event_base_t, int32_t, void* data) {
  const auto* value = static_cast<const esp_netif_sntp_time_sync_t*>(data);
  event_us = value->tv.tv_sec * 1000000LL + value->tv.tv_usec;
  ++events;
}

// Waits on task publication with a bounded host-tick budget, not a UTC
// deadline.
void Await(const std::atomic<int>& count, int expected) {
  for (int i = 0; i < 1000 && count.load() < expected; ++i) vTaskDelay(1);
  ASSERT_GE(count.load(), expected);
}

class NetifSntp : public testing::Test {
 protected:
  void SetUp() override {
    host::Reset();
    roo_testing::system::ResetToPowerOnMapping();
    notifications = 0;
    events = 0;
    deinit_on_notify = false;
    ASSERT_EQ(ESP_OK, esp_event_loop_create_default());
    ASSERT_EQ(ESP_OK,
              esp_event_handler_register(NETIF_SNTP_EVENT, NETIF_SNTP_TIME_SYNC,
                                         SyncEvent, nullptr));
    host::ServerBehavior server;
    server.time_source = host::TimeSource::kReference;
    server.reference_unix_us = 1700000000123456LL;
    server.response_delay_us = 5000;
    ASSERT_EQ(ESP_OK, host::ConfigureServer(server));
  }
  void TearDown() override {
    host::Reset();
    EXPECT_EQ(ESP_OK, esp_event_handler_unregister(
                          NETIF_SNTP_EVENT, NETIF_SNTP_TIME_SYNC, SyncEvent));
    EXPECT_EQ(ESP_OK, esp_event_loop_delete_default());
    roo_testing::system::ResetToPowerOnMapping();
  }
};

// Verifies default initialization delivers a waiter, user hook, and copied
// event.
TEST_F(NetifSntp, DefaultPathAndEventPayload) {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("virtual.invalid");
  config.sync_cb = Notification;
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_netif_sntp_init(&config));
  if (!system_time_is_auto_sync_enabled()) {
    EXPECT_EQ(ESP_ERR_TIMEOUT, esp_netif_sntp_sync_wait(0));
  }
  system_time_delay_micros(5000);
  ASSERT_EQ(ESP_OK, esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000)));
  Await(notifications, 1);
  Await(events, 1);
  EXPECT_EQ(notified_us.load(), event_us.load());
  EXPECT_EQ(SNTP_SYNC_STATUS_COMPLETED, esp_sntp_get_sync_status());
  unsigned reachability;
  EXPECT_EQ(ESP_OK, esp_netif_sntp_reachability(0, &reachability));
  EXPECT_EQ(1u, reachability);
}

// Verifies deferred start and disabled waiting keep independent API semantics.
TEST_F(NetifSntp, DeferredStartWithoutWaiter) {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("virtual.invalid");
  config.start = false;
  config.wait_for_sync = false;
  config.sync_cb = Notification;
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  EXPECT_FALSE(esp_sntp_enabled());
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_netif_sntp_sync_wait(0));
  system_time_delay_micros(10000);
  EXPECT_EQ(0, notifications.load());
  ASSERT_EQ(ESP_OK, esp_netif_sntp_start());
  system_time_delay_micros(5000);
  Await(notifications, 1);
}

// Verifies unsupported settings cannot partially install or replace a client.
TEST_F(NetifSntp, UnsupportedFlagsAreAtomic) {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("virtual.invalid");
  config.smooth_sync = true;
  EXPECT_EQ(ESP_ERR_NOT_SUPPORTED, esp_netif_sntp_init(&config));
  EXPECT_FALSE(esp_sntp_enabled());
  config.smooth_sync = false;
  config.server_from_dhcp = true;
  EXPECT_EQ(ESP_ERR_NOT_SUPPORTED, esp_netif_sntp_init(&config));
  config.server_from_dhcp = false;
  config.renew_servers_after_new_IP = true;
  EXPECT_EQ(ESP_ERR_NOT_SUPPORTED, esp_netif_sntp_init(&config));
  config.renew_servers_after_new_IP = false;
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  config.smooth_sync = true;
  EXPECT_EQ(ESP_ERR_NOT_SUPPORTED, esp_netif_sntp_init(&config));
  EXPECT_TRUE(esp_sntp_enabled());
  EXPECT_STREQ("virtual.invalid", esp_sntp_getservername(0));
}

// Verifies deinit from a notification does not wait on itself or lose event
// data.
TEST_F(NetifSntp, DeinitInsideCallback) {
  deinit_on_notify = true;
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("virtual.invalid");
  config.sync_cb = Notification;
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  system_time_delay_micros(5000);
  Await(notifications, 1);
  Await(events, 1);
  EXPECT_FALSE(esp_sntp_enabled());
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_netif_sntp_sync_wait(0));
  deinit_on_notify = false;
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  system_time_delay_micros(5000);
  Await(notifications, 2);
}

struct Waiter {
  std::atomic<bool> entered{false};
  std::atomic<esp_err_t> result{ESP_FAIL};
  SemaphoreHandle_t finished;
};
void WaitUntilDeinit(void* data) {
  auto* waiter = static_cast<Waiter*>(data);
  waiter->entered = true;
  waiter->result = esp_netif_sntp_sync_wait(portMAX_DELAY);
  xSemaphoreGive(waiter->finished);
  vTaskDelete(nullptr);
}

// Verifies all indefinite waiters wake on deinit while their storage stays
// alive.
TEST_F(NetifSntp, DeinitUnblocksMultipleWaiters) {
  host::ServerBehavior silent;
  silent.responds = false;
  ASSERT_EQ(ESP_OK, host::ConfigureServer(silent));
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("silent.invalid");
  ASSERT_EQ(ESP_OK, esp_netif_sntp_init(&config));
  Waiter waiters[2];
  for (Waiter& waiter : waiters) {
    waiter.finished = xSemaphoreCreateBinary();
    ASSERT_NE(nullptr, waiter.finished);
    ASSERT_EQ(pdPASS, xTaskCreate(WaitUntilDeinit, "sntp-wait", 8192, &waiter,
                                  tskIDLE_PRIORITY + 2, nullptr));
  }
  for (Waiter& waiter : waiters) {
    for (int i = 0; i < 1000 && !waiter.entered.load(); ++i) vTaskDelay(1);
    ASSERT_TRUE(waiter.entered.load());
  }
  esp_netif_sntp_deinit();
  for (Waiter& waiter : waiters) {
    ASSERT_EQ(pdTRUE, xSemaphoreTake(waiter.finished, pdMS_TO_TICKS(1000)));
    EXPECT_EQ(ESP_ERR_INVALID_STATE, waiter.result.load());
    vSemaphoreDelete(waiter.finished);
  }
}
}  // namespace
