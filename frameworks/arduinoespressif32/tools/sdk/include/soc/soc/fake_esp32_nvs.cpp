#include "fake_esp32_nvs.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <fstream>
#include <set>

#include "esp32-hal-spi.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/spi_struct.h"

#include "google/protobuf/text_format.h"
#include "nvs_flash.h"

using namespace google::protobuf;

Nvs::Nvs(const std::string& path) : path_(path), next_id_(0) {
  if (!path_.empty()) {
    std::ifstream input_file(path);
    if (!input_file.is_open()) {
      std::cerr << "WARNING: could not open the NVS storage file - '" << path
                << "'" << std::endl;
      return;
    }
    std::cout << "NVS storage file opened: " << path << std::endl;
    std::string val((std::istreambuf_iterator<char>(input_file)),
                    std::istreambuf_iterator<char>());

    if (!TextFormat::ParseFromString(val, &storage_)) {
      std::cerr << "Could not parse - '" << path << "'" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

void Nvs::save() {
  std::string val;
  if (!TextFormat::PrintToString(storage_, &val)) {
    std::cerr << "Could not print" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::ofstream output_file(path_);
  output_file << val;
  output_file.close();
}

esp_err_t Nvs::init(const char* partition_name) {
  (*storage_.mutable_partitions())[partition_name];
  save();
  return ESP_OK;
}

esp_err_t Nvs::open(const char* part_name, const char* name, bool readonly,
                    nvs_handle_t* out_handle) {
  if (!storage_.mutable_partitions()->contains(part_name)) {
    return ESP_ERR_NVS_PART_NOT_FOUND;
  }
  roo_testing::esp32::nvs::Partition& part =
      (*storage_.mutable_partitions())[part_name];
  if (!part.mutable_name_spaces()->contains(name)) {
    if (readonly) {
      return ESP_ERR_NVS_NOT_FOUND;
    }
    (*part.mutable_name_spaces())[name];
  }
  int id = next_id_++;
  Handle handle = {
      .partition_name = part_name,
      .ns_name = name,
      .readonly = readonly,
  };
  open_partitions_[id] = handle;
  *out_handle = id;
  return ESP_OK;
}

esp_err_t Nvs::set(nvs_handle_t handle, const char* key,
                   roo_testing::esp32::nvs::EntryValue val) {
  if (open_partitions_.find(handle) == open_partitions_.end()) {
    return ESP_ERR_NVS_INVALID_HANDLE;
  }
  Handle& h = open_partitions_[handle];
  if (h.readonly) {
    return ESP_ERR_NVS_READ_ONLY;
  }
  if (strlen(key) > 15) {
    return ESP_ERR_NVS_INVALID_NAME;
  }
  (*(*(*storage_.mutable_partitions())[h.partition_name]
          .mutable_name_spaces())[h.ns_name]
        .mutable_entries())[key] = std::move(val);
  return ESP_OK;
}

esp_err_t Nvs::set_i8(nvs_handle_t handle, const char* key, int8_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I8);
  val.set_sint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U8);
  val.set_uint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i16(nvs_handle_t handle, const char* key, int16_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I16);
  val.set_sint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U16);
  val.set_uint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i32(nvs_handle_t handle, const char* key, int32_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I32);
  val.set_sint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U32);
  val.set_uint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i64(nvs_handle_t handle, const char* key, int64_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I64);
  val.set_sint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U64);
  val.set_uint_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_str(nvs_handle_t handle, const char* key,
                       const char* value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::STR);
  val.set_str_value(value);
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::set_blob(nvs_handle_t handle, const char* key, const void* value,
                        size_t length) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::BLOB);
  val.set_blob_value(std::string((const char*)value, length));
  return set(handle, key, std::move(val));
}

esp_err_t Nvs::get(nvs_handle_t handle, const char* key,
                   roo_testing::esp32::nvs::Type expected_type,
                   roo_testing::esp32::nvs::EntryValue& val) {
  if (open_partitions_.find(handle) == open_partitions_.end()) {
    return ESP_ERR_NVS_INVALID_HANDLE;
  }
  Handle& h = open_partitions_[handle];
  if (strlen(key) > 15) {
    return ESP_ERR_NVS_INVALID_NAME;
  }
  auto& entries = *(*(*storage_.mutable_partitions())[h.partition_name]
                         .mutable_name_spaces())[h.ns_name]
                       .mutable_entries();
  if (!entries.contains(key)) {
    return ESP_ERR_NVS_NOT_FOUND;
  }
  const auto& entry = entries[key];
  if (entry.type() != expected_type) {
    return ESP_ERR_NVS_TYPE_MISMATCH;
  }
  val = entry;
  return ESP_OK;
}

esp_err_t Nvs::get_i8(nvs_handle_t handle, const char* key, int8_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::I8, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u8(nvs_handle_t handle, const char* key, uint8_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::U8, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i16(nvs_handle_t handle, const char* key, int16_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::I16, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u16(nvs_handle_t handle, const char* key, uint16_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::U16, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i32(nvs_handle_t handle, const char* key, int32_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::I32, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u32(nvs_handle_t handle, const char* key, uint32_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::U32, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i64(nvs_handle_t handle, const char* key, int64_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::I64, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u64(nvs_handle_t handle, const char* key, uint64_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::U64, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_str(nvs_handle_t handle, const char* key, char* value,
                       size_t* length) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::STR, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.str_value().size());
  memcpy(value, val.str_value().c_str(), actual);
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::get_blob(nvs_handle_t handle, const char* key, char* value,
                        size_t* length) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = get(handle, key, roo_testing::esp32::nvs::BLOB, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.blob_value().size());
  memcpy(value, val.blob_value().c_str(), actual);
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::commit() {
  save();
  return ESP_OK;
}

void Nvs::close(nvs_handle_t handle) { open_partitions_.erase(handle); }

esp_err_t Nvs::erase_key(nvs_handle_t handle, const char* key) {
  if (open_partitions_.find(handle) == open_partitions_.end()) {
    return ESP_ERR_NVS_INVALID_HANDLE;
  }
  Handle& h = open_partitions_[handle];
  if (h.readonly) {
    return ESP_ERR_NVS_READ_ONLY;
  }
  if (strlen(key) > 15) {
    return ESP_ERR_NVS_INVALID_NAME;
  }
  (*(*(*storage_.mutable_partitions())[h.partition_name]
          .mutable_name_spaces())[h.ns_name]
        .mutable_entries())
      .erase(key);
  return ESP_OK;
}

esp_err_t Nvs::erase_all(nvs_handle_t handle) {
  if (open_partitions_.find(handle) == open_partitions_.end()) {
    return ESP_ERR_NVS_INVALID_HANDLE;
  }
  Handle& h = open_partitions_[handle];
  if (h.readonly) {
    return ESP_ERR_NVS_READ_ONLY;
  }
  (*(*storage_.mutable_partitions())[h.partition_name]
        .mutable_name_spaces())[h.ns_name]
      .mutable_entries()->clear();
  return ESP_OK;
}
