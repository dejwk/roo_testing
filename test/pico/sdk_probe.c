#include <inttypes.h>
#include <stdio.h>

#include "pico/time.h"
#include "test/pico/profile_contract.h"

// Verifies C SDK headers, libc stdio, both uptime widths, and shared host
// clocks compile and link together without IDF or target assembly.
int main(void) {
  const uint64_t before = time_us_64();
  const uint32_t low = time_us_32();
  const uint64_t after = to_us_since_boot(get_absolute_time());
  if (after < before || after - before >= UINT32_MAX) return 1;
  if ((uint32_t)(low - (uint32_t)before) > after - before) return 2;
  if (get_core_num() != 0) return 3;
  if (hardware_alarm_claim_unused(false) != -1) return 4;
  printf("Pico SDK %s host probe: uptime=%" PRIu64 " us, core=0\n",
         PICO_SDK_VERSION_STRING, after);
  return 0;
}
