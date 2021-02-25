#include <chrono>
#include <thread>

#include "clock.h"

namespace testing_transducers {

Clock* getDefaultSystemClock() {
  static SystemClock clock;
  return &clock;
}

}  // namespace
