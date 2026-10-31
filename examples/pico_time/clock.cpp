#include <cinttypes>
#include <cstdio>

#include "pico/time.h"

int main() {
  const uint64_t start_us = time_us_64();
  for (int i = 0; i < 3; ++i) {
    sleep_ms(10);
    std::printf("Pico uptime: %" PRIu64 " us since start\n",
                time_us_64() - start_us);
  }
  return 0;
}
