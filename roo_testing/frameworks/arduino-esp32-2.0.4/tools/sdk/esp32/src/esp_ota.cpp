
#include <glog/logging.h>

#include "esp_err.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

extern "C" {

int esp_ota_get_app_elf_sha256(char* dst, size_t size) { return 0; }

esp_err_t esp_ota_begin(const esp_partition_t* partition, size_t image_size,
                        esp_ota_handle_t* out_handle) {
  return ESP_OK;
}

esp_err_t esp_ota_write(esp_ota_handle_t handle, const void* data,
                        size_t size) {
  return ESP_OK;
}

esp_err_t esp_ota_write_with_offset(esp_ota_handle_t handle, const void* data,
                                    size_t size, uint32_t offset) {
  return ESP_OK;
}

esp_err_t esp_ota_end(esp_ota_handle_t handle) { return ESP_OK; }

esp_err_t esp_ota_abort(esp_ota_handle_t handle) { return ESP_OK; }

esp_err_t esp_ota_set_boot_partition(const esp_partition_t* partition) {
  return ESP_OK;
}

const esp_partition_t* esp_ota_get_boot_partition(void) { return nullptr; }

const esp_partition_t* esp_ota_get_running_partition(void) { return nullptr; }

const esp_partition_t* esp_ota_get_next_update_partition(
    const esp_partition_t* start_from) {
  return nullptr;
}

esp_err_t esp_ota_get_partition_description(const esp_partition_t* partition,
                                            esp_app_desc_t* app_desc) {
  return ESP_OK;
}

uint8_t esp_ota_get_app_partition_count(void) { return 1; }

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) { return ESP_OK; }

esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void) { return ESP_OK; }

const esp_partition_t* esp_ota_get_last_invalid_partition(void) {
  return nullptr;
}

esp_err_t esp_ota_get_state_partition(const esp_partition_t* partition,
                                      esp_ota_img_states_t* ota_state) {
  return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_ota_erase_last_boot_app_partition(void) { return ESP_OK; }

bool esp_ota_check_rollback_is_possible(void) { return false; }
}
