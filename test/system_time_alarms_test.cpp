#include <cstdint>
#include <limits>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

// Verifies manual advancement delivers alarms in deadline and registration
// order.
TEST(SystemTimeAlarmsTest, DeliversChronologicallyDuringManualDelay) {
  const int64_t start_us = system_time_get_micros();
  std::vector<int> deliveries;
  ScheduleSystemTimeAlarm(start_us + 5, [&] { deliveries.push_back(5); });
  ScheduleSystemTimeAlarm(start_us + 2, [&] { deliveries.push_back(2); });
  ScheduleSystemTimeAlarm(start_us + 5, [&] { deliveries.push_back(6); });

  system_time_delay_micros(10);

  EXPECT_EQ((std::vector<int>{2, 5, 6}), deliveries);
  EXPECT_EQ(start_us + 10, system_time_get_micros());
}

// Verifies due work is explicitly pumped and pending work can be cancelled.
TEST(SystemTimeAlarmsTest, PumpsDueWorkAndCancelsPendingWork) {
  const int64_t now_us = system_time_get_micros();
  bool due_delivered = false;
  bool cancelled_delivered = false;
  ScheduleSystemTimeAlarm(now_us, [&] { due_delivered = true; });
  const SystemTimeAlarmId cancelled =
      ScheduleSystemTimeAlarm(now_us, [&] { cancelled_delivered = true; });

  CancelSystemTimeAlarm(cancelled);
  EXPECT_FALSE(due_delivered);
  ProcessSystemTimeAlarms();

  EXPECT_TRUE(due_delivered);
  EXPECT_FALSE(cancelled_delivered);
}

// Verifies registration queues already-due work until an explicit pump.
TEST(SystemTimeAlarmsTest, QueuesPastDeadlineUntilExplicitPump) {
  const int64_t now_us = system_time_get_micros();
  bool delivered = false;

  ScheduleSystemTimeAlarm(now_us - 1, [&] { delivered = true; });

  EXPECT_FALSE(delivered);
  ProcessSystemTimeAlarms();
  EXPECT_TRUE(delivered);
}

// Verifies invalid alarm deadlines fail before modifying alarm state.
TEST(SystemTimeAlarmsTest, RejectsUnrepresentableDeadline) {
  const int64_t now_us = system_time_get_micros();
  bool delivered = false;
  ScheduleSystemTimeAlarm(now_us, [&] { delivered = true; });

  EXPECT_DEATH(
      ScheduleSystemTimeAlarm(std::numeric_limits<int64_t>::max(), [] {}), "");
  ProcessSystemTimeAlarms();

  EXPECT_TRUE(delivered);
}

// Verifies callback-created due work is drained without recursive delivery.
TEST(SystemTimeAlarmsTest, DrainsWorkCreatedByCallback) {
  const int64_t now_us = system_time_get_micros();
  std::vector<int> deliveries;
  ScheduleSystemTimeAlarm(now_us, [&] {
    deliveries.push_back(1);
    ScheduleSystemTimeAlarm(now_us, [&] { deliveries.push_back(2); });
    ProcessSystemTimeAlarms();
  });

  ProcessSystemTimeAlarms();

  EXPECT_EQ((std::vector<int>{1, 2}), deliveries);
}

}  // namespace
