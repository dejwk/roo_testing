#include <chrono>
#include <thread>

#include "clock.h"

namespace roo_testing_transducers {

Clock* getDefaultSystemClock() {
  static SystemClock clock;
  return &clock;
}

}  // namespace roo_testing_transducers
