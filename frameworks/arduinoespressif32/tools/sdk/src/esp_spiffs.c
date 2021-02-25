// Copyright 2015-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_spiffs.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

bool spiffs_mounted = false;

/**
 * Register and mount SPIFFS to VFS with given path prefix.
 *
 * @param   conf                      Pointer to esp_vfs_spiffs_conf_t configuration structure
 *
 * @return  
 *          - ESP_OK                  if success
 *          - ESP_ERR_NO_MEM          if objects could not be allocated
 *          - ESP_ERR_INVALID_STATE   if already mounted or partition is encrypted
 *          - ESP_ERR_NOT_FOUND       if partition for SPIFFS was not found
 *          - ESP_FAIL                if mount or format fails
 */
esp_err_t esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t * conf) {
  if (spiffs_mounted) {
    return ESP_ERR_INVALID_STATE;
  }
  spiffs_mounted = true;
  return ESP_OK;
}

/**
 * Unregister and unmount SPIFFS from VFS
 *
 * @param partition_label  Optional, label of the partition to unregister.
 *                         If not specified, first partition with subtype=spiffs is used.
 *
 * @return  
 *          - ESP_OK if successful
 *          - ESP_ERR_INVALID_STATE already unregistered
 */
esp_err_t esp_vfs_spiffs_unregister(const char* partition_label) {
  if (!spiffs_mounted) {
    return ESP_ERR_INVALID_STATE;
  }
  spiffs_mounted = false;
  return ESP_OK;
}

/**
 * Check if SPIFFS is mounted
 *
 * @param partition_label  Optional, label of the partition to check.
 *                         If not specified, first partition with subtype=spiffs is used.
 *
 * @return  
 *          - true    if mounted
 *          - false   if not mounted
 */
bool esp_spiffs_mounted(const char* partition_label) {
  return spiffs_mounted;
}

/**
 * Format the SPIFFS partition
 *
 * @param partition_label  Optional, label of the partition to format.
 *                         If not specified, first partition with subtype=spiffs is used.
 * @return  
 *          - ESP_OK      if successful
 *          - ESP_FAIL    on error
 */
esp_err_t esp_spiffs_format(const char* partition_label) {
  return ESP_OK;
}

/**
 * Get information for SPIFFS
 *
 * @param partition_label           Optional, label of the partition to get info for.
 *                                  If not specified, first partition with subtype=spiffs is used.
 * @param[out] total_bytes          Size of the file system
 * @param[out] used_bytes           Current used bytes in the file system
 *
 * @return  
 *          - ESP_OK                  if success
 *          - ESP_ERR_INVALID_STATE   if not mounted
 */
esp_err_t esp_spiffs_info(const char* partition_label, size_t *total_bytes, size_t *used_bytes) {
  *total_bytes = 0;
  *used_bytes = 0;
  return ESP_OK;
}

#ifdef __cplusplus
}
#endif
