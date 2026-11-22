#include <chrono>
#include <thread>

#include "gtest/gtest.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include "roo_testing/system/timer.h"

namespace {

// Verifies host passage is visible through Pico without explicit progression.
TEST(PicoAutoTimeTest, FollowsHostTimeWithoutAnExplicitPump) {
  const uint64_t before_us = time_us_64();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  EXPECT_GE(time_us_64(), before_us + 2000);
}

// Verifies an SDK sleep paces against the host instead of only advancing a
// counter.
TEST(PicoAutoTimeTest, SleepPacesAgainstHostTime) {
  const std::chrono::steady_clock::time_point before =
      std::chrono::steady_clock::now();
  sleep_ms(10);
  EXPECT_GE(std::chrono::steady_clock::now() - before,
            std::chrono::milliseconds(9));
}

// Verifies busy-until rechecks an auto clock whose floor was advanced
// explicitly.
TEST(PicoAutoTimeTest, BusyWaitsUntilThePublishedUptimeDeadline) {
  sleep_us(100);
  const absolute_time_t deadline = make_timeout_time_us(100);
  busy_wait_until(deadline);
  EXPECT_TRUE(time_reached(deadline));
}

// Verifies host-paced Pico waits deliver timer IRQs on the waiting core.
TEST(PicoAutoTimeTest, TimerIrqWakesEventWaitInCoreContext) {
  __sev();
  __wfe();
  unsigned exception = 0;
  ASSERT_GT(add_alarm_in_ms(
                5,
                [](alarm_id_t, void* data) -> int64_t {
                  *static_cast<unsigned*>(data) = __get_current_exception();
                  return 0;
                },
                &exception, false),
            0);
  const absolute_time_t timeout = make_timeout_time_ms(500);
  while (exception == 0 && !best_effort_wfe_or_timeout(timeout)) {
  }
  EXPECT_EQ(exception, 16u + PICO_TIME_DEFAULT_ALARM_POOL_HARDWARE_ALARM_NUM);
  EXPECT_FALSE(time_reached(timeout));
}

// Verifies a host-side device callback can wake a blocked Pico core.
TEST(PicoAutoTimeTest, SharedDeviceEventWakesHostWait) {
  __sev();
  __wfe();
  const absolute_time_t timeout = make_timeout_time_ms(500);
  const SystemTimeAlarmId alarm =
      ScheduleSystemTimeAlarm(system_time_get_micros() + 5000, [] { __sev(); });
  EXPECT_FALSE(best_effort_wfe_or_timeout(timeout));
  EXPECT_FALSE(time_reached(timeout));
  CancelSystemTimeAlarm(alarm);
}

}  // namespace
