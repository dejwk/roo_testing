#include <Arduino.h>

#include <cstdlib>

#include "sketch_support.h"

#if ROO_TESTING_SKETCH_COPT != 29
#error "roo_arduino_example did not forward copts"
#endif

#if ROO_TESTING_SKETCH_DEFINE != 71
#error "roo_arduino_example did not forward defines"
#endif

void setup() {
  std::exit(sketch_support_value() == 42 ? 0 : 1);
}

void loop() {}
