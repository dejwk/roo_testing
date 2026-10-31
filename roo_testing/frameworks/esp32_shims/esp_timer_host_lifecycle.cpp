#include "esp_timer.h"
#include "roo_testing/frameworks/esp32_shims/esp_timer_host.h"
#include "roo_testing/frameworks/esp32_shims/sntp_host.h"

namespace roo_testing::esp32_shims {

bool IsEspTimerImplInitializedForHost();

esp_err_t InitializeEspTimerForHost() {
  esp_err_t err = esp_timer_early_init();
  if (err != ESP_OK || IsEspTimerImplInitializedForHost()) return err;
  err = esp_timer_init();
  if (err == ESP_OK) InitializeSntpForHost();
  return err;
}

esp_err_t ShutdownEspTimerForHost() {
  if (!IsEspTimerImplInitializedForHost()) return ESP_OK;
  const esp_err_t err = ShutdownSntpForHost();
  if (err != ESP_OK) return err;
  return esp_timer_deinit();
}

}  // namespace roo_testing::esp32_shims
