#include <gtest/gtest.h>

#include "esp_sntp.h"
// Verifies unsupported void-API modes fail explicitly rather than fake success.
TEST(SntpUnsupported, RejectsUnsupportedModes) {
  EXPECT_DEATH(esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH),
               "smooth synchronization is unsupported");
  EXPECT_DEATH(esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_LISTENONLY),
               "listen-only mode is unsupported");
}
