#include <atomic>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

std::atomic<int> callback_count{0};
std::atomic<bool> callback_in_isr{true};

void TimerCallback(void*) {
  callback_in_isr.store(xPortInIsrContext(), std::memory_order_relaxed);
  callback_count.fetch_add(1, std::memory_order_relaxed);
}

// Verifies runner initialization dispatches public callbacks from the timer
// task.
TEST(EspTimerHostTest, DispatchesOneShotCallbackFromTimerTask) {
  callback_count.store(0, std::memory_order_relaxed);
  callback_in_isr.store(true, std::memory_order_relaxed);
  const esp_timer_create_args_t args = {
      .callback = TimerCallback,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "one-shot",
      .skip_unhandled_events = false,
  };
  esp_timer_handle_t timer = nullptr;
  ASSERT_EQ(ESP_OK, esp_timer_create(&args, &timer));
  ASSERT_EQ(ESP_OK, esp_timer_start_once(timer, 1000));

  system_time_delay_micros(5000);

  EXPECT_EQ(1, callback_count.load(std::memory_order_relaxed));
  EXPECT_FALSE(callback_in_isr.load(std::memory_order_relaxed));
  EXPECT_FALSE(esp_timer_is_active(timer));
  EXPECT_EQ(ESP_OK, esp_timer_delete(timer));
  // Deletion is deferred through the vendored timer task.
  system_time_delay_micros(0);
}

}  // namespace
