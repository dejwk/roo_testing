#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include <glog/logging.h>

#include "roo_testing/system/timer.h"

extern "C" {

esp_err_t esp_task_wdt_init(uint32_t timeout, bool panic) { return ESP_OK; }

esp_err_t esp_task_wdt_deinit(void) { return ESP_OK; }

esp_err_t esp_task_wdt_add(TaskHandle_t handle) { return ESP_OK; }

esp_err_t esp_task_wdt_reset(void) { return ESP_OK; }

esp_err_t esp_task_wdt_delete(TaskHandle_t handle) { return ESP_OK; }

esp_err_t esp_task_wdt_status(TaskHandle_t handle) { return ESP_OK; }

int64_t esp_timer_get_time() {
  return SystemTimer().getTimeMicros();
}

void emulatedDelayMicroseconds(uint64_t micros) {
  SystemTimer().delayMicros(micros);
}

// esp_err_t nvs_flash_init(void) { return ESP_OK; }


/** @cond */
void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression) {
  LOG(FATAL) << "Check failed with " << rc << " for expression " << expression << ", in function " << function << "(" << file << ":" << line << ")";
  exit(rc);
}

/** @cond */
void _esp_error_check_failed_without_abort(esp_err_t rc, const char *file, int line, const char *function, const char *expression) {
  LOG(ERROR) << "Check failed with " << rc << " for expression " << expression << ", in function " << function << "(" << file << ":" << line << ")";
}

uint8_t temprature_sens_read() { return 75; }

void esp_timer_impl_update_apb_freq(uint32_t apb_ticks_per_us) {}

}
