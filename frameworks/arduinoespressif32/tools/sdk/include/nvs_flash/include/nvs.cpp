#include <fstream>
#include <iostream>
#include <map>

#include "nvs.h"

#include "soc/fake_esp32.h"

extern "C" {

esp_err_t nvs_flash_init_partition(const char* partition_label) {
  std::cerr << "Called init partition " << partition_label;
  return FakeEsp32().nvs.init(partition_label);
}

esp_err_t nvs_open(const char* name, nvs_open_mode_t open_mode,
                   nvs_handle_t* out_handle) {
  FakeEsp32().nvs.init("nvs");
  return nvs_open_from_partition("nvs", name, open_mode, out_handle);
}

esp_err_t nvs_open_from_partition(const char* part_name, const char* name,
                                  nvs_open_mode_t open_mode,
                                  nvs_handle_t* out_handle) {
  return FakeEsp32().nvs.open(part_name, name, (open_mode == NVS_READONLY),
                              out_handle);
}

esp_err_t nvs_set_i8(nvs_handle_t handle, const char* key, int8_t value) {
  return FakeEsp32().nvs.set_i8(handle, key, value);
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
  return FakeEsp32().nvs.set_u8(handle, key, value);
}

esp_err_t nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value) {
  return FakeEsp32().nvs.set_i16(handle, key, value);
}

esp_err_t nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
  return FakeEsp32().nvs.set_u16(handle, key, value);
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value) {
  return FakeEsp32().nvs.set_i32(handle, key, value);
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
  return FakeEsp32().nvs.set_u32(handle, key, value);
}

esp_err_t nvs_set_i64(nvs_handle_t handle, const char* key, int64_t value) {
  return FakeEsp32().nvs.set_i64(handle, key, value);
}

esp_err_t nvs_set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
  return FakeEsp32().nvs.set_u64(handle, key, value);
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char* key, const char* value) {
  return FakeEsp32().nvs.set_str(handle, key, value);
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value,
                       size_t length) {
  return FakeEsp32().nvs.set_blob(handle, key, value, length);
}

esp_err_t nvs_get_i8(nvs_handle_t handle, const char* key, int8_t* out_value) {
  return FakeEsp32().nvs.get_i8(handle, key, out_value);
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value) {
  return FakeEsp32().nvs.get_u8(handle, key, out_value);
}

esp_err_t nvs_get_i16(nvs_handle_t handle, const char* key,
                      int16_t* out_value) {
  return FakeEsp32().nvs.get_i16(handle, key, out_value);
}

esp_err_t nvs_get_u16(nvs_handle_t handle, const char* key,
                      uint16_t* out_value) {
  return FakeEsp32().nvs.get_u16(handle, key, out_value);
}

esp_err_t nvs_get_i32(nvs_handle_t handle, const char* key,
                      int32_t* out_value) {
  return FakeEsp32().nvs.get_i32(handle, key, out_value);
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char* key,
                      uint32_t* out_value) {
  return FakeEsp32().nvs.get_u32(handle, key, out_value);
}

esp_err_t nvs_get_i64(nvs_handle_t handle, const char* key,
                      int64_t* out_value) {
  return FakeEsp32().nvs.get_i64(handle, key, out_value);
}

esp_err_t nvs_get_u64(nvs_handle_t handle, const char* key,
                      uint64_t* out_value) {
  return FakeEsp32().nvs.get_u64(handle, key, out_value);
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char* key, char* out_value,
                      size_t* length) {
  return FakeEsp32().nvs.get_str(handle, key, out_value, length);
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value,
                       size_t* length) {
  return FakeEsp32().nvs.get_blob(handle, key, (char*)out_value, length);
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key) {
  return FakeEsp32().nvs.erase_key(handle, key);
}

esp_err_t nvs_erase_all(nvs_handle_t handle) {
  return FakeEsp32().nvs.erase_all(handle);
}

esp_err_t nvs_commit(nvs_handle_t handle) { return FakeEsp32().nvs.commit(); }

void nvs_close(nvs_handle_t handle) { FakeEsp32().nvs.close(handle); }

esp_err_t nvs_get_stats(const char* part_name, nvs_stats_t* nvs_stats) {
  nvs_stats->free_entries = 1000;
  return ESP_OK;
}

}
