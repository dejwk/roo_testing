#include <stdlib.h>
#include <time.h>

#include "esp32-hal.h"
#include "esp_netif.h"
#include "esp_sntp.h"

namespace {
void SetTimeZone(long offset, int daylight) {
  char standard[32] = {};
  char summer[32] = "DST";
  char zone[64] = {};
  if (offset % 3600 != 0) {
    snprintf(standard, sizeof(standard), "UTC%ld:%02ld:%02ld", offset / 3600,
             labs((offset % 3600) / 60), labs(offset % 60));
  } else {
    snprintf(standard, sizeof(standard), "UTC%ld", offset / 3600);
  }
  if (daylight != 3600) {
    const long dst = offset - daylight;
    if (dst % 3600 != 0) {
      snprintf(summer, sizeof(summer), "DST%ld:%02ld:%02ld", dst / 3600,
               labs((dst % 3600) / 60), labs(dst % 60));
    } else {
      snprintf(summer, sizeof(summer), "DST%ld", dst / 3600);
    }
  }
  snprintf(zone, sizeof(zone), "%s%s", standard, summer);
  setenv("TZ", zone, 1);
  tzset();
}
// All frontends use the same client; configuration alone does not set UTC.
void ConfigureSntp(const char* server1, const char* server2,
                   const char* server3) {
  esp_sntp_stop();
  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, server1);
  esp_sntp_setservername(1, server2);
  esp_sntp_setservername(2, server3);
  esp_sntp_init();
}
}  // namespace

extern "C" {

void configTime(long gmt_offset, int daylight_offset, const char* server1,
                const char* server2, const char* server3) {
  esp_netif_init();
  SetTimeZone(-gmt_offset, daylight_offset);
  ConfigureSntp(server1, server2, server3);
}

void configTzTime(const char* tz, const char* server1, const char* server2,
                  const char* server3) {
  esp_netif_init();
  if (tz != nullptr) setenv("TZ", tz, 1);
  tzset();
  ConfigureSntp(server1, server2, server3);
}

bool getLocalTime(struct tm* info, uint32_t timeout_ms) {
  if (info == nullptr) return false;
  const unsigned long start = millis();
  do {
    const time_t now = time(nullptr);
    localtime_r(&now, info);
    if (info->tm_year > 116) return true;
    delay(10);
  } while (millis() - start <= timeout_ms);
  return false;
}

}  // extern "C"
