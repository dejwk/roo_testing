#include <chrono>
#include <thread>

#include "clock.h"

namespace testing_transducers {

int64_t SystemClock::getTimeMicros() const {
  std::chrono::time_point<std::chrono::steady_clock> ts =
      std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             ts.time_since_epoch())
      .count();
}

void SystemClock::delayMicros(uint64_t delay) {
  std::this_thread::sleep_for(std::chrono::microseconds(delay));
}

}  // namespace testing_transducers
