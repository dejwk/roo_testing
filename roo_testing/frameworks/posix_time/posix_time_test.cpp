#include "roo_testing/frameworks/posix_time/posix_time.h"

#include <errno.h>
#include <gtest/gtest.h>
#include <sys/time.h>
#include <time.h>

#include "roo_testing/system/timer.h"
#include "roo_testing/system/wall_clock.h"

namespace {
using namespace roo_testing::system;

// Verifies a late first read still counts from boot, without importing host
// UTC.
TEST(PosixTime, ColdBootAndLateRead) {
  ResetToPowerOnMapping();
  system_time_delay_micros(2500000);
  const int64_t before = system_time_get_micros();
  struct timeval tv;
  ASSERT_EQ(0, gettimeofday(&tv, nullptr));
  const int64_t read = tv.tv_sec * 1000000LL + tv.tv_usec;
  EXPECT_GE(read, before);
  EXPECT_LE(read, system_time_get_micros());
  time_t output;
  EXPECT_GE(time(&output), tv.tv_sec);
  EXPECT_LT(output, 1000000);  // Epoch-era, not today's date.
}

// Verifies all public read paths share a correction while uptime stays
// unchanged.
TEST(PosixTime, StepsAndNegativeEpoch) {
  const int64_t before = system_time_get_micros();
  const struct timeval value = {-1, 750000};
  ASSERT_EQ(0, settimeofday(&value, nullptr));
  struct timespec ts;
  ASSERT_EQ(0, clock_gettime(CLOCK_REALTIME, &ts));
  EXPECT_EQ(-1, ts.tv_sec);
  EXPECT_GE(ts.tv_nsec, 750000000);
  EXPECT_LT(system_time_get_micros() - before, 1000000);
  if (!system_time_is_auto_sync_enabled()) {
    EXPECT_EQ(750000000, ts.tv_nsec);
    EXPECT_EQ(-1, time(nullptr));
    system_time_delay_micros(250000);
    EXPECT_EQ(0, time(nullptr));
  }
  ResetToPowerOnMapping();
}

// Verifies invalid writes and overflowing reads do not alter state or host
// time.
TEST(PosixTime, CheckedBoundaries) {
  const struct timeval bad = {1, 1000000};
  EXPECT_EQ(-1, settimeofday(&bad, nullptr));
  EXPECT_EQ(EINVAL, errno);
  ASSERT_TRUE(SetUnixTimeMicros(INT64_MAX));
  system_time_delay_micros(1);
  struct timeval tv;
  EXPECT_EQ(-1, gettimeofday(&tv, nullptr));
  EXPECT_EQ(EOVERFLOW, errno);
  const struct timespec value = {0, 0};
  EXPECT_EQ(-1, clock_settime(CLOCK_MONOTONIC, &value));
  EXPECT_EQ(ENOTSUP, errno);
  EXPECT_EQ(-1, adjtime(nullptr, nullptr));
  EXPECT_EQ(ENOTSUP, errno);
  ResetToPowerOnMapping();
}

// Verifies native monotonic reads bypass the mapping and host UTC is
// server-only.
TEST(PosixTime, HostClockIsolation) {
  int64_t host_before;
  ASSERT_TRUE(GetHostUnixTimeMicros(host_before));
  ASSERT_TRUE(SetUnixTimeMicros(0));
  struct timespec monotonic;
  ASSERT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &monotonic));
  EXPECT_GT(monotonic.tv_sec, 0);
  int64_t host_after;
  ASSERT_TRUE(GetHostUnixTimeMicros(host_after));
  EXPECT_GE(host_after, host_before);
  EXPECT_GT(host_after, 1000000000000000LL);
  struct timespec resolution;
  EXPECT_EQ(0, clock_getres(CLOCK_REALTIME, &resolution));
  EXPECT_EQ(1000, resolution.tv_nsec);
  ResetToPowerOnMapping();
}
}  // namespace
