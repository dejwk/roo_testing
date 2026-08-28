#include <limits>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

namespace {

// Verifies concurrent explicit advances publish one monotonic manual uptime.
TEST(TimePublicationTest, PublishesConcurrentManualAdvances) {
  constexpr int kThreadCount = 4;
  constexpr int kAdvancesPerThread = 100;
  const int64_t start_us = system_time_get_micros();

  std::vector<std::thread> threads;
  for (int i = 0; i < kThreadCount; ++i) {
    threads.emplace_back([] {
      for (int j = 0; j < kAdvancesPerThread; ++j) {
        system_time_delay_micros(1);
      }
    });
  }
  for (std::thread& thread : threads) thread.join();

  EXPECT_EQ(start_us + kThreadCount * kAdvancesPerThread,
            system_time_get_micros());
}

// Verifies an unrepresentable lag leaves the published uptime unchanged.
TEST(TimePublicationTest, RejectsUnrepresentableLagBeforePublication) {
  const int64_t before_us = system_time_get_micros();
  EXPECT_DEATH(system_time_lag_ns(std::numeric_limits<uint64_t>::max()), "");
  EXPECT_EQ(before_us, system_time_get_micros());
}

}  // namespace
