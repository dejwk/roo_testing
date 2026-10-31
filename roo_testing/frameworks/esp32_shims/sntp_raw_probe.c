#include <string.h>

#include "lwip/apps/sntp.h"

// Compiles the raw lwIP header independently of ESP-IDF's inline aliases.
int SntpRawProbe(void) {
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "raw.invalid");
  sntp_init();
  int valid = sntp_enabled() != 0 &&
              sntp_getoperatingmode() == SNTP_OPMODE_POLL &&
              strcmp(sntp_getservername(0), "raw.invalid") == 0 &&
              sntp_getserver(0) != NULL;
#if SNTP_MONITOR_SERVER_REACHABILITY
  valid = valid && sntp_getreachability(0) == 0;
#endif
  sntp_stop();
  return valid && sntp_enabled() == 0;
}
