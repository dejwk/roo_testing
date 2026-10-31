#include "pico/time.h"

// Verifies unavailable sleep implementations do not silently link as no-ops.
int main(void) {
  sleep_us(1);
  return 0;
}
