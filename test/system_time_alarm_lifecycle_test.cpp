#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

// Verifies host shutdown joins the auto-sync worker and closes registration.
TEST(SystemTimeAlarmLifecycleTest, JoinsWorkerAndStopsService) {
  ASSERT_TRUE(system_time_is_auto_sync_enabled());
  ScheduleSystemTimeAlarm(system_time_get_micros() + 1000000, [] {});

  ASSERT_TRUE(TryBeginSystemTimeServiceShutdownForHost());
  FinishSystemTimeServiceShutdownForHost();

  EXPECT_FALSE(TryBeginSystemTimeServiceShutdownForHost());
  EXPECT_DEATH(ScheduleSystemTimeAlarm(system_time_get_micros(), [] {}), "");
}

}  // namespace
