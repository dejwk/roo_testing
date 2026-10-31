#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>

#include "gtest/gtest.h"
#include "pico/time.h"
#include "roo_testing/system/timer.h"

namespace {

constexpr uint64_t kMaxClockMicros = static_cast<uint64_t>(INT64_MAX / 1000);

// Verifies a bad unsigned duration fails before changing published time.
TEST(PicoTimeDeathTest, RejectsUnrepresentableRelativeDurations) {
  const uint64_t before_us = time_us_64();
  EXPECT_DEATH(sleep_us(std::numeric_limits<uint64_t>::max()),
               "Pico duration exceeds");
  EXPECT_DEATH(busy_wait_us(std::numeric_limits<uint64_t>::max()), "");
  EXPECT_EQ(time_us_64(), before_us);
}

// Verifies finite clock limits are checked without turning end-of-time into
// nil.
TEST(PicoTimeDeathTest, RejectsUnrepresentableDeadlinesAndInfiniteWaits) {
  const uint64_t before_us = time_us_64();
  EXPECT_DEATH(sleep_until(from_us_since_boot(kMaxClockMicros + 1)),
               "Pico deadline exceeds");
  EXPECT_DEATH(sleep_until(at_the_end_of_time), "Pico deadline exceeds");
  EXPECT_DEATH(busy_wait_until(at_the_end_of_time), "Pico deadline exceeds");
  EXPECT_DEATH(best_effort_wfe_or_timeout(at_the_end_of_time),
               "Pico deadline exceeds");
  EXPECT_EQ(time_us_64(), before_us);
}

// Verifies the last whole microsecond representable by the shared clock works.
TEST(PicoTimeDeathTest, AcceptsTheLastRepresentableMicrosecond) {
  ASSERT_FALSE(system_time_is_auto_sync_enabled());
  EXPECT_EXIT(
      {
        sleep_us(kMaxClockMicros - time_us_64());
        if (time_us_64() != kMaxClockMicros) std::_Exit(1);
        sleep_until(from_us_since_boot(kMaxClockMicros));
        std::_Exit(0);
      },
      testing::ExitedWithCode(0), "");
}

// Verifies addition overflow at the shared clock boundary is rejected too.
TEST(PicoTimeDeathTest, RejectsDurationThatOverflowsTheCurrentUptime) {
  ASSERT_FALSE(system_time_is_auto_sync_enabled());
  EXPECT_DEATH(
      {
        sleep_us(kMaxClockMicros - time_us_64());
        sleep_us(1);
      },
      "Check failed");
}

// Verifies waits advance manual time exactly and preserve both counter widths.
TEST(PicoManualTimeTest, AdvancesExactlyAndTruncatesThe32BitCounter) {
  ASSERT_FALSE(system_time_is_auto_sync_enabled());
  const uint64_t before_us = time_us_64();
  sleep_us((uint64_t{1} << 32) + 17);
  EXPECT_EQ(time_us_64(), before_us + (uint64_t{1} << 32) + 17);
  EXPECT_EQ(time_us_32(), static_cast<uint32_t>(time_us_64()));
  const uint64_t before_ms_wait = time_us_64();
  sleep_ms(std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(time_us_64(), before_ms_wait + uint64_t{UINT32_MAX} * 1000);
}

// Verifies sub-microsecond clock advances use truncation rather than rounding.
TEST(PicoManualTimeTest, ReadsTheSharedSubMicrosecondFloor) {
  const int64_t before_us = system_time_get_micros();
  system_time_lag_ns(999);
  EXPECT_EQ(time_us_64(), static_cast<uint64_t>(system_time_get_micros()));
  EXPECT_LE(time_us_64(), static_cast<uint64_t>(before_us + 1));
}

// Verifies SDK sleep is a chronological shared-alarm pump, with no SDK IRQ
// claim.
TEST(PicoManualTimeTest, DeliversSharedDeviceDeadlinesDuringSleep) {
  const int64_t before_us = system_time_get_micros();
  int64_t observed_us = -1;
  ScheduleSystemTimeAlarm(before_us + 10, [&] {
    observed_us = static_cast<int64_t>(time_us_64());
  });
  sleep_us(20);
  EXPECT_EQ(observed_us, before_us + 10);
  EXPECT_EQ(time_us_64(), static_cast<uint64_t>(before_us + 20));
}

// Verifies host busy waits neither advance manual uptime nor deliver alarms.
TEST(PicoManualTimeTest, BusyWaitsPreserveManualClockAndPendingWork) {
  const uint64_t before_us = time_us_64();
  bool delivered = false;
  const SystemTimeAlarmId alarm = ScheduleSystemTimeAlarm(
      static_cast<int64_t>(before_us), [&] { delivered = true; });
  const std::chrono::steady_clock::time_point host_before =
      std::chrono::steady_clock::now();
  busy_wait_us_32(100);
  busy_wait_us(100);
  busy_wait_ms(1);
  busy_wait_until(from_us_since_boot(before_us + 100));
  EXPECT_GE(std::chrono::steady_clock::now() - host_before,
            std::chrono::microseconds(1300));
  EXPECT_EQ(time_us_64(), before_us);
  EXPECT_FALSE(delivered);
  CancelSystemTimeAlarm(alarm);
}

}  // namespace
