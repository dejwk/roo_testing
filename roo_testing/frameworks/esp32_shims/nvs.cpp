#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

constexpr char kDefaultPartition[] = "nvs";

Nvs& Storage() { return FakeEsp32().nvs; }

}  // namespace

extern "C" {

esp_err_t nvs_flash_init(void) { return Storage().init(kDefaultPartition); }

esp_err_t nvs_flash_init_partition(const char* partition_label) {
  if (partition_label == nullptr) return ESP_ERR_INVALID_ARG;
  return Storage().init(partition_label);
}

esp_err_t nvs_flash_init_partition_ptr(const esp_partition_t* partition) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  return Storage().init(partition->label);
}

esp_err_t nvs_flash_deinit(void) { return ESP_OK; }
esp_err_t nvs_flash_deinit_partition(const char*) { return ESP_OK; }
esp_err_t nvs_flash_erase(void) { return ESP_OK; }
esp_err_t nvs_flash_erase_partition(const char*) { return ESP_OK; }
esp_err_t nvs_flash_erase_partition_ptr(const esp_partition_t* partition) {
  return partition == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
esp_err_t nvs_flash_secure_init(nvs_sec_cfg_t*) { return nvs_flash_init(); }
esp_err_t nvs_flash_secure_init_partition(const char* partition,
                                          nvs_sec_cfg_t*) {
  return nvs_flash_init_partition(partition);
}

esp_err_t nvs_open(const char* namespace_name, nvs_open_mode_t open_mode,
                   nvs_handle_t* out_handle) {
  return Storage().open(kDefaultPartition, namespace_name,
                        open_mode == NVS_READONLY, out_handle);
}

esp_err_t nvs_open_from_partition(const char* part_name,
                                  const char* namespace_name,
                                  nvs_open_mode_t open_mode,
                                  nvs_handle_t* out_handle) {
  if (part_name == nullptr || namespace_name == nullptr ||
      out_handle == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  return Storage().open(part_name, namespace_name, open_mode == NVS_READONLY,
                        out_handle);
}

esp_err_t nvs_set_i8(nvs_handle_t h, const char* k, int8_t v) {
  return Storage().set_i8(h, k, v);
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char* k, uint8_t v) {
  return Storage().set_u8(h, k, v);
}
esp_err_t nvs_set_i16(nvs_handle_t h, const char* k, int16_t v) {
  return Storage().set_i16(h, k, v);
}
esp_err_t nvs_set_u16(nvs_handle_t h, const char* k, uint16_t v) {
  return Storage().set_u16(h, k, v);
}
esp_err_t nvs_set_i32(nvs_handle_t h, const char* k, int32_t v) {
  return Storage().set_i32(h, k, v);
}
esp_err_t nvs_set_u32(nvs_handle_t h, const char* k, uint32_t v) {
  return Storage().set_u32(h, k, v);
}
esp_err_t nvs_set_i64(nvs_handle_t h, const char* k, int64_t v) {
  return Storage().set_i64(h, k, v);
}
esp_err_t nvs_set_u64(nvs_handle_t h, const char* k, uint64_t v) {
  return Storage().set_u64(h, k, v);
}
esp_err_t nvs_set_str(nvs_handle_t h, const char* k, const char* v) {
  return Storage().set_str(h, k, v);
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char* k, const void* v, size_t n) {
  return Storage().set_blob(h, k, v, n);
}

esp_err_t nvs_get_i8(nvs_handle_t h, const char* k, int8_t* v) {
  return Storage().get_i8(h, k, v);
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char* k, uint8_t* v) {
  return Storage().get_u8(h, k, v);
}
esp_err_t nvs_get_i16(nvs_handle_t h, const char* k, int16_t* v) {
  return Storage().get_i16(h, k, v);
}
esp_err_t nvs_get_u16(nvs_handle_t h, const char* k, uint16_t* v) {
  return Storage().get_u16(h, k, v);
}
esp_err_t nvs_get_i32(nvs_handle_t h, const char* k, int32_t* v) {
  return Storage().get_i32(h, k, v);
}
esp_err_t nvs_get_u32(nvs_handle_t h, const char* k, uint32_t* v) {
  return Storage().get_u32(h, k, v);
}
esp_err_t nvs_get_i64(nvs_handle_t h, const char* k, int64_t* v) {
  return Storage().get_i64(h, k, v);
}
esp_err_t nvs_get_u64(nvs_handle_t h, const char* k, uint64_t* v) {
  return Storage().get_u64(h, k, v);
}
esp_err_t nvs_get_str(nvs_handle_t h, const char* k, char* v, size_t* n) {
  return Storage().get_str(h, k, v, n);
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char* k, void* v, size_t* n) {
  return Storage().get_blob(h, k, static_cast<char*>(v), n);
}

esp_err_t nvs_erase_key(nvs_handle_t h, const char* k) {
  return Storage().erase_key(h, k);
}
esp_err_t nvs_erase_all(nvs_handle_t h) { return Storage().erase_all(h); }
esp_err_t nvs_commit(nvs_handle_t) { return Storage().commit(); }
void nvs_close(nvs_handle_t h) { Storage().close(h); }

esp_err_t nvs_get_stats(const char*, nvs_stats_t* stats) {
  if (stats == nullptr) return ESP_ERR_INVALID_ARG;
  memset(stats, 0, sizeof(*stats));
  stats->total_entries = 1024;
  stats->free_entries = 1024;
  return ESP_OK;
}

esp_err_t nvs_get_used_entry_count(nvs_handle_t, size_t* used_entries) {
  if (used_entries == nullptr) return ESP_ERR_INVALID_ARG;
  *used_entries = 0;
  return ESP_OK;
}

}  // extern "C"
