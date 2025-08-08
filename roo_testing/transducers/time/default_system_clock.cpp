#include "default_system_clock.h"
#include "roo_testing/system/timer.h"

namespace roo_testing_transducers {

int64_t SystemClock::getTimeMicros() const {
  return system_time_get_micros();
  // std::chrono::time_point<std::chrono::steady_clock> ts =
  //     std::chrono::steady_clock::now();
  // return std::chrono::duration_cast<std::chrono::microseconds>(
  //            ts.time_since_epoch())
  //     .count();
}

void SystemClock::delayMicros(uint64_t delay) {
  system_time_delay_micros(delay);
  // roo_testing::this_thread::sleep_for(std::chrono::microseconds(delay));
}

Clock* getDefaultSystemClock() {
  static SystemClock clock;
  return &clock;
}

}  // namespace roo_testing_transducers
