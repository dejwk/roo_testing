#include <atomic>

#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "rom/ets_sys.h"
#include "roo_testing/system/timer.h"

namespace {

std::atomic<int> callback_count{0};
std::atomic<bool> callback_in_isr{true};

void TimerCallback(void*) {
  callback_in_isr.store(xPortInIsrContext(), std::memory_order_relaxed);
  callback_count.fetch_add(1, std::memory_order_relaxed);
}

// Verifies the vendored ETS adapter delivers one-shot and periodic timers.
TEST(EtsTimerLegacyTest, DeliversOneShotAndPeriodicCallbacks) {
  callback_count.store(0, std::memory_order_relaxed);
  callback_in_isr.store(true, std::memory_order_relaxed);
  ETSTimer timer = {};
  ets_timer_setfn(&timer, TimerCallback, nullptr);
  ets_timer_arm_us(&timer, 1000, false);
  system_time_delay_micros(5000);
  EXPECT_EQ(1, callback_count.load(std::memory_order_relaxed));
  EXPECT_FALSE(callback_in_isr.load(std::memory_order_relaxed));

  ets_timer_arm_us(&timer, 1000, true);
  system_time_delay_micros(5000);
  EXPECT_GE(callback_count.load(std::memory_order_relaxed), 2);
  ets_timer_disarm(&timer);
  ets_timer_done(&timer);
  // ets_timer_done is represented by a deferred esp_timer deletion.
  system_time_delay_micros(0);
}

}  // namespace
