#include <cstdlib>
#include <cstring>
#include <thread>

#include "Arduino.h"
#include "glog/logging.h"
#include "roo_testing/system/timer.h"

int main(int argc, char** argv) {
  uint64_t iterations = UINT64_MAX;
  if (argc == 2) {
    CHECK_EQ(std::strncmp(argv[1], "--iterations=", 13), 0);
    char* end;
    iterations = std::strtoull(argv[1] + 13, &end, 10);
    CHECK(end != argv[1] + 13 && *end == '\0') << "Invalid iteration count";
  } else {
    CHECK_EQ(argc, 1);
  }
  init();
  initVariant();
  Serial.begin(115200);
  setup();
  for (uint64_t i = 0; i < iterations; ++i) {
    loop();
    yield();
  }
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return 0;
}
