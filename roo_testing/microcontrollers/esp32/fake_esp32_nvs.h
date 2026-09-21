#pragma once

#include <inttypes.h>

#include <string>
#include <vector>

// #include "esp_err.h"
// #include "nvs.h"

typedef int32_t esp_err_t;
typedef uint32_t nvs_handle_t;

class NvsImpl;

class Nvs {
 public:
  // Values mirror nvs_type_t without making the storage model depend on
  // ESP-IDF headers.
  enum class Type : uint8_t {
    U8 = 0x01,
    I8 = 0x11,
    U16 = 0x02,
    I16 = 0x12,
    U32 = 0x04,
    I32 = 0x14,
    U64 = 0x08,
    I64 = 0x18,
    STR = 0x21,
    BLOB = 0x42,
  };

  struct EntryInfo {
    std::string namespace_name;
    std::string key;
    Type type;
  };

  struct Stats {
    size_t used_entries;
    size_t free_entries;
    size_t available_entries;
    size_t total_entries;
    size_t namespace_count;
  };

  Nvs(const std::string& path);

  ~Nvs();

  esp_err_t init(const char* partition_name, size_t partition_size = 0);

  esp_err_t deinit(const char* partition_name);

  esp_err_t erase_partition(const char* partition_name);

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

  esp_err_t find_key(nvs_handle_t handle, const char* key, Type* out_type);

  esp_err_t list_entries(const char* part_name, const char* namespace_name,
                         int type, std::vector<EntryInfo>* entries);
  esp_err_t list_entries(nvs_handle_t handle, int type,
                         std::vector<EntryInfo>* entries);

  esp_err_t get_stats(const char* partition_name, Stats* stats);
  esp_err_t get_used_entry_count(nvs_handle_t handle, size_t* used_entries);

  esp_err_t commit(nvs_handle_t handle);

  esp_err_t erase_key(nvs_handle_t handle, const char* key);

  esp_err_t erase_all(nvs_handle_t handle);

  esp_err_t purge_all(nvs_handle_t handle);

  void close(nvs_handle_t handle);

 private:
  void save();

  NvsImpl* impl_;
};
