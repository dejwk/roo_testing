#include <cstdlib>
#include <cstring>
#include <thread>

#include "Arduino.h"
#include "glog/logging.h"
#include "roo_testing/system/timer.h"
#if defined(__FREERTOS)
#include "roo_testing/frameworks/freertos_posix/runner.h"
extern void setup1() __attribute__((weak));
extern void loop1() __attribute__((weak));
#endif

namespace {
int RunSketch(void* context) {
  const uint64_t iterations = *static_cast<const uint64_t*>(context);
  init();
  initVariant();
  Serial.begin(115200);
  setup();
  for (uint64_t i = 0; i < iterations; ++i) {
    loop();
    yield();
  }
  return 0;
}
}  // namespace

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
#if defined(__FREERTOS)
  CHECK(setup1 == nullptr && loop1 == nullptr)
      << "Unimplemented: Arduino-Pico FreeRTOS core 1";
  return roo_testing::freertos_posix::Run(RunSketch, &iterations);
#else
  const int result = RunSketch(&iterations);
  while (!TryBeginSystemTimeServiceShutdownForHost()) std::this_thread::yield();
  FinishSystemTimeServiceShutdownForHost();
  return result;
#endif
}
