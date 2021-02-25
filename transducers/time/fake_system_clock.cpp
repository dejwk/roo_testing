#include <chrono>
#include <thread>

#include "clock.h"

namespace testing_transducers {

Clock* getDefaultSystemClock() {
  static FakeClock clock;
  return &clock;
}

}  // namespace
