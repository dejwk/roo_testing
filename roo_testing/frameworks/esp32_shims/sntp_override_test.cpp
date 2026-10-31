#include <gtest/gtest.h>

#include <atomic>

#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

std::atomic<int> overrides{0};
extern "C" void sntp_sync_time(struct timeval*) { ++overrides; }

// Verifies a firmware override owns publication, without a hidden clock update.
TEST(SntpOverride, OwnsPublication) {
  roo_testing::esp32::sntp::ServerBehavior behavior;
  behavior.response_delay_us = 1000;
  ASSERT_EQ(ESP_OK, roo_testing::esp32::sntp::ConfigureServer(behavior));
  esp_sntp_setservername(0, "virtual.invalid");
  esp_sntp_init();
  system_time_delay_micros(1000);
  for (int i = 0; i < 1000 && overrides.load() == 0; ++i) vTaskDelay(1);
  EXPECT_EQ(1, overrides.load());
  int64_t utc;
  ASSERT_TRUE(roo_testing::system::GetUnixTimeMicros(utc));
  EXPECT_LT(utc, 1000000000000LL);
  roo_testing::esp32::sntp::Reset();
}
