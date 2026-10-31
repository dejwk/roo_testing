#include <chrono>
#include <thread>

#include "gtest/gtest.h"
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

}  // namespace
