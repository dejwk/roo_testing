#include <gtest/gtest.h>

#include "roo_testing/system/timer.h"

TEST(SimpleExampleTest, TimerAdvances) {
  system_time_set_auto_sync(false);
  auto start = system_time_get_micros();
  system_time_delay_micros(5000);
  auto end = system_time_get_micros();
  system_time_set_auto_sync(true);
  EXPECT_GE(end - start, 5000);
}
