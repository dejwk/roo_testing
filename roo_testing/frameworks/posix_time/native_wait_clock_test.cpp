#include <gtest/gtest.h>
#include <time.h>

#include "roo_testing/system/wall_clock.h"
extern "C" {
#include "roo_testing/frameworks/esp-idf/components/freertos/FreeRTOS-Kernel/portable/linux/utils/wait_for_event.h"
}
namespace {
int64_t NativeNanos() {
  struct timespec now;
  EXPECT_EQ(0, clock_gettime(CLOCK_MONOTONIC, &now));
  return now.tv_sec * 1000000000LL + now.tv_nsec;
}

// Verifies UTC steps never turn native waits into immediate or enormous waits.
TEST(NativeWaitClock, IndependentOfDeviceUtc) {
  struct event* ev = event_create();
  for (int64_t utc : {0LL, 4000000000000000LL, -1000000000000LL}) {
    ASSERT_TRUE(roo_testing::system::SetUnixTimeMicros(utc));
    const int64_t started = NativeNanos();
    EXPECT_FALSE(event_wait_timed(ev, 20));
    const int64_t elapsed = NativeNanos() - started;
    EXPECT_GE(elapsed, 20000000);
    EXPECT_LT(elapsed, 2000000000);
    event_signal(ev);
    EXPECT_TRUE(event_wait_timed(ev, 20));
  }
  event_delete(ev);
  roo_testing::system::ResetToPowerOnMapping();
}

// Verifies deadlines crossing a second boundary keep normalized nanoseconds.
TEST(NativeWaitClock, NormalizesDeadlineCarry) {
  struct event* ev = event_create();
  const int64_t started = NativeNanos();
  EXPECT_FALSE(event_wait_timed(ev, 1100));
  EXPECT_GE(NativeNanos() - started, 1100000000);
  event_delete(ev);
}
}  // namespace
