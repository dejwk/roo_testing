#include <thread>

#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"
#if defined(__FREERTOS)
#include "roo_testing/frameworks/freertos_posix/runner.h"
namespace {
int RunTests(void*) { return RUN_ALL_TESTS(); }
}  // namespace
#endif

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  system_time_is_auto_sync_enabled();
#if defined(__FREERTOS)
  return roo_testing::freertos_posix::Run(RunTests);
#else
  const int result = RUN_ALL_TESTS();
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return result;
#endif
}
