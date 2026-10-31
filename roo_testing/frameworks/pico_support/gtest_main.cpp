#include <thread>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  // Fix the link-selected mode before tests without rebasing uptime observed
  // by global constructors. SDK-only tests run on this native thread.
  system_time_is_auto_sync_enabled();
  const int result = RUN_ALL_TESTS();
  while (!TryBeginSystemTimeServiceShutdownForHost()) {
    std::this_thread::yield();
  }
  FinishSystemTimeServiceShutdownForHost();
  return result;
}
