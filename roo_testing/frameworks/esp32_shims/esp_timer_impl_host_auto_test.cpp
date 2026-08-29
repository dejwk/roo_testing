#include <atomic>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

extern "C" {
#include "components/esp_timer/private_include/esp_timer_impl.h"
}

namespace {

std::atomic<int> handler_calls{0};

void AlarmHandler(void*) {
  handler_calls.fetch_add(1, std::memory_order_relaxed);
}

// Verifies auto-sync alarm delivery reaches the common handler through IRQ.
TEST(EspTimerImplHostAutoTest, DeliversWithoutAnExplicitAlarmPump) {
  handler_calls.store(0, std::memory_order_relaxed);
  ASSERT_EQ(ESP_OK, esp_timer_impl_early_init());
  ASSERT_EQ(ESP_OK, esp_timer_impl_init(AlarmHandler));

  esp_timer_impl_set_alarm(
      static_cast<uint64_t>(system_time_get_micros() + 1000));
  system_time_delay_micros(5000);

  EXPECT_EQ(1, handler_calls.load(std::memory_order_relaxed));
  esp_timer_impl_deinit();
}

}  // namespace
