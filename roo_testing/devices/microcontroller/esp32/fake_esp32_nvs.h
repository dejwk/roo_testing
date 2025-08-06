#pragma once

#include <inttypes.h>

#include <string>

// #include "esp_err.h"
// #include "nvs.h"

typedef int32_t esp_err_t;
typedef uint32_t nvs_handle_t;

class NvsImpl;

class Nvs {
 public:
  Nvs(const std::string& path);

  ~Nvs();

  esp_err_t init(const char* partition_name);

  esp_err_t open(const char* part_name, const char* name, bool readonly,
                 nvs_handle_t* out_handle);

  esp_err_t set_i8(nvs_handle_t handle, const char* key, int8_t value);
  esp_err_t set_u8(nvs_handle_t handle, const char* key, uint8_t value);
  esp_err_t set_i16(nvs_handle_t handle, const char* key, int16_t value);
  esp_err_t set_u16(nvs_handle_t handle, const char* key, uint16_t value);
  esp_err_t set_i32(nvs_handle_t handle, const char* key, int32_t value);
  esp_err_t set_u32(nvs_handle_t handle, const char* key, uint32_t value);
  esp_err_t set_i64(nvs_handle_t handle, const char* key, int64_t value);
  esp_err_t set_u64(nvs_handle_t handle, const char* key, uint64_t value);

  esp_err_t set_str(nvs_handle_t handle, const char* key, const char* value);
  esp_err_t set_blob(nvs_handle_t handle, const char* key, const void* value,
                     size_t length);

  esp_err_t get_i8(nvs_handle_t handle, const char* key, int8_t* value);
  esp_err_t get_u8(nvs_handle_t handle, const char* key, uint8_t* value);
  esp_err_t get_i16(nvs_handle_t handle, const char* key, int16_t* value);
  esp_err_t get_u16(nvs_handle_t handle, const char* key, uint16_t* value);
  esp_err_t get_i32(nvs_handle_t handle, const char* key, int32_t* value);
  esp_err_t get_u32(nvs_handle_t handle, const char* key, uint32_t* value);
  esp_err_t get_i64(nvs_handle_t handle, const char* key, int64_t* value);
  esp_err_t get_u64(nvs_handle_t handle, const char* key, uint64_t* value);

  esp_err_t get_str(nvs_handle_t handle, const char* key, char* value,
                    size_t* length);

  esp_err_t get_blob(nvs_handle_t handle, const char* key, char* value,
                     size_t* length);

  esp_err_t commit();

  esp_err_t erase_key(nvs_handle_t handle, const char* key);

  esp_err_t erase_all(nvs_handle_t handle);

  void close(nvs_handle_t handle);

 private:
  void save();

  NvsImpl* impl_;
};
