#include "pico/time.h"

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

const uint64_t kPicoFromConstructor = time_us_64();
const int64_t kSharedFromConstructor = system_time_get_micros();
const bool kAutoFromConstructor = system_time_is_auto_sync_enabled();
const uint64_t kNilFromConstructor = to_us_since_boot(nil_time);
const uint64_t kEndFromConstructor = to_us_since_boot(at_the_end_of_time);

// Verifies SDK reads and sentinels are safe before the runner initializes
// tests.
TEST(PicoTimeTest, ReadsLinkSelectedClockAndSentinelsBeforeMain) {
  EXPECT_EQ(kAutoFromConstructor, PICO_TEST_AUTO != 0);
  EXPECT_EQ(kNilFromConstructor, uint64_t{0});
  EXPECT_EQ(kEndFromConstructor, static_cast<uint64_t>(INT64_MAX));
  EXPECT_LE(kPicoFromConstructor,
            static_cast<uint64_t>(kSharedFromConstructor));
  if (PICO_TEST_AUTO == 0) {
    EXPECT_EQ(kPicoFromConstructor, uint64_t{0});
    EXPECT_EQ(kSharedFromConstructor, 0);
  }
}

// Verifies SDK inline timestamp reads observe the same published uptime.
TEST(PicoTimeTest, SharesUptimeWithTheEmulator) {
  const uint64_t before_us = static_cast<uint64_t>(system_time_get_micros());
  const uint64_t pico_us = to_us_since_boot(get_absolute_time());
  const uint64_t after_us = static_cast<uint64_t>(system_time_get_micros());
  EXPECT_LE(before_us, pico_us);
  EXPECT_LE(pico_us, after_us);
  if (PICO_TEST_AUTO == 0) {
    EXPECT_EQ(before_us, after_us);
  }
}

// Verifies sentinel comparisons and SDK arithmetic retain their public values.
TEST(PicoTimeTest, PreservesSentinelsAndSaturatingTimestampArithmetic) {
  EXPECT_TRUE(is_nil_time(nil_time));
  EXPECT_TRUE(is_at_the_end_of_time(at_the_end_of_time));
  EXPECT_TRUE(time_reached(nil_time));
  EXPECT_FALSE(time_reached(at_the_end_of_time));
  EXPECT_EQ(to_us_since_boot(delayed_by_ms(from_us_since_boot(1), 2)), 2001u);
  EXPECT_EQ(to_ms_64_since_boot(from_us_since_boot(4294967296000ULL)),
            4294967296ULL);
  EXPECT_EQ(to_ms_since_boot(from_us_since_boot(4294967296000ULL)), 0u);
  EXPECT_EQ(
      absolute_time_diff_us(from_us_since_boot(10), from_us_since_boot(7)), -3);
  EXPECT_EQ(to_us_since_boot(absolute_time_min(at_the_end_of_time, nil_time)),
            0u);
  EXPECT_TRUE(is_at_the_end_of_time(delayed_by_us(at_the_end_of_time, 1)));
  EXPECT_TRUE(is_at_the_end_of_time(delayed_by_us(
      from_us_since_boot(1), std::numeric_limits<uint64_t>::max())));
  EXPECT_TRUE(is_at_the_end_of_time(delayed_by_ms(at_the_end_of_time, 1)));
  EXPECT_TRUE(is_at_the_end_of_time(
      make_timeout_time_us(std::numeric_limits<uint64_t>::max())));
}

// Verifies SDK sleeps and timeout helpers return only after their uptime
// target.
TEST(PicoTimeTest, SleepsInMicrosecondsMillisecondsAndUntilADeadline) {
  uint64_t before_us = time_us_64();
  sleep_us(123);
  EXPECT_GE(time_us_64(), before_us + 123);
  before_us = time_us_64();
  sleep_ms(2);
  EXPECT_GE(time_us_64(), before_us + 2000);
  const absolute_time_t deadline = make_timeout_time_us(321);
  sleep_until(deadline);
  EXPECT_TRUE(time_reached(deadline));
  const absolute_time_t timeout = make_timeout_time_ms(1);
  EXPECT_TRUE(best_effort_wfe_or_timeout(timeout));
  EXPECT_TRUE(time_reached(timeout));
}

// Verifies zero, nil, and past waits do not manufacture a new future deadline.
TEST(PicoTimeTest, AcceptsZeroAndPastWaits) {
  const uint64_t before_us = time_us_64();
  sleep_us(0);
  sleep_ms(0);
  sleep_until(nil_time);
  busy_wait_until(nil_time);
  EXPECT_TRUE(best_effort_wfe_or_timeout(nil_time));
  if (PICO_TEST_AUTO == 0) {
    EXPECT_EQ(time_us_64(), before_us);
  }
}

}  // namespace
