#include <atomic>
#include <chrono>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

// Verifies the native waiter delivers an auto-sync alarm while its caller
// spins.
TEST(SystemTimeAutoAlarmsTest, DeliversWithoutExplicitPump) {
  ASSERT_TRUE(system_time_is_auto_sync_enabled());
  std::atomic<bool> delivered{false};
  const int64_t deadline_us = system_time_get_micros() + 10000;
  ScheduleSystemTimeAlarm(
      deadline_us, [&] { delivered.store(true, std::memory_order_release); });

  const std::chrono::steady_clock::time_point timeout =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (!delivered.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < timeout) {
  }

  EXPECT_TRUE(delivered.load(std::memory_order_acquire));
}

}  // namespace
