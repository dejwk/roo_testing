#include <thread>

#include "Arduino.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"
#if defined(__FREERTOS)
#include "roo_testing/frameworks/freertos_posix/runner.h"
#endif

namespace {
int RunTests(void*) {
  init();
  initVariant();
  Serial.begin(115200);
  return RUN_ALL_TESTS();
}
}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
#if defined(__FREERTOS)
  return roo_testing::freertos_posix::Run(RunTests);
#else
  const int result = RunTests(nullptr);
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return result;
#endif
}
