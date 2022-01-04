#include <chrono>
#include <thread>

#include "clock.h"

namespace roo_testing_transducers {

Clock* getDefaultSystemClock() {
  static FakeClock clock;
  return &clock;
}

}  // namespace roo_testing_transducers
