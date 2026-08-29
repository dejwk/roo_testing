#include "esp_timer.h"
#include "roo_testing/frameworks/esp32_shims/esp_timer_host.h"

namespace roo_testing::esp32_shims {

bool IsEspTimerImplInitializedForHost();

esp_err_t InitializeEspTimerForHost() {
  esp_err_t err = esp_timer_early_init();
  if (err != ESP_OK || IsEspTimerImplInitializedForHost()) return err;
  return esp_timer_init();
}

esp_err_t ShutdownEspTimerForHost() {
  if (!IsEspTimerImplInitializedForHost()) return ESP_OK;
  return esp_timer_deinit();
}

}  // namespace roo_testing::esp32_shims
