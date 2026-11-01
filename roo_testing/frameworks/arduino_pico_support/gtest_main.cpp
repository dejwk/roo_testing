#include <thread>

#include "Arduino.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  init();
  initVariant();
  Serial.begin(115200);
  const int result = RUN_ALL_TESTS();
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return result;
}
