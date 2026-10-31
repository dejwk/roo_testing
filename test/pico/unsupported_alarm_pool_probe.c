#include "pico/time.h"

// Verifies unavailable SDK alarm pools do not link as successful placeholders.
int main(void) {
  alarm_pool_init_default();
  return 0;
}
