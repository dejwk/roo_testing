#include "esp_err.h"
#include "esp_etm.h"

extern "C" esp_err_t esp_timer_new_etm_alarm_event(
    esp_etm_event_handle_t* out_event) {
  if (out_event != nullptr) *out_event = nullptr;
  return ESP_ERR_NOT_SUPPORTED;
}
