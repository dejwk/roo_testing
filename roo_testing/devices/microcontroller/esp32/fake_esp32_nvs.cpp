#include "fake_esp32_nvs.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <random>
#include <set>

#include "roo_testing/devices/microcontroller/esp32/storage.pb.h"

// #include "esp32-hal-spi.h"
// #include "soc/gpio_sig_map.h"
// #include "soc/gpio_struct.h"
// #include "soc/spi_struct.h"

#include "google/protobuf/text_format.h"
// #include "nvs_flash.h"

using namespace google::protobuf;

#define ESP_OK 0

#define ESP_ERR_NVS_BASE 0x1100 /*!< Starting number of error codes */
#define ESP_ERR_NVS_NOT_INITIALIZED \
  (ESP_ERR_NVS_BASE + 0x01) /*!< The storage driver is not initialized */
#define ESP_ERR_NVS_NOT_FOUND \
  (ESP_ERR_NVS_BASE +         \
   0x02) /*!< Id namespace doesn’t exist yet and mode is NVS_READONLY */
#define ESP_ERR_NVS_TYPE_MISMATCH                                         \
  (ESP_ERR_NVS_BASE + 0x03) /*!< The type of set or get operation doesn't \
                               match the type of value stored in NVS */
#define ESP_ERR_NVS_READ_ONLY \
  (ESP_ERR_NVS_BASE + 0x04) /*!< Storage handle was opened as read only */
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE                                         \
  (ESP_ERR_NVS_BASE + 0x05) /*!< There is not enough space in the underlying \
                               storage to save the value */
#define ESP_ERR_NVS_INVALID_NAME \
  (ESP_ERR_NVS_BASE + 0x06) /*!< Namespace name doesn’t satisfy constraints */
#define ESP_ERR_NVS_INVALID_HANDLE \
  (ESP_ERR_NVS_BASE + 0x07) /*!< Handle has been closed or is NULL */
#define ESP_ERR_NVS_REMOVE_FAILED                                              \
  (ESP_ERR_NVS_BASE +                                                          \
   0x08) /*!< The value wasn’t updated because flash write operation has     \
            failed. The value was written however, and update will be finished \
            after re-initialization of nvs, provided that flash operation      \
            doesn’t fail again. */
#define ESP_ERR_NVS_KEY_TOO_LONG \
  (ESP_ERR_NVS_BASE + 0x09) /*!< Key name is too long */
#define ESP_ERR_NVS_PAGE_FULL \
  (ESP_ERR_NVS_BASE +         \
   0x0a) /*!< Internal error; never returned by nvs API functions */
#define ESP_ERR_NVS_INVALID_STATE                                           \
  (ESP_ERR_NVS_BASE +                                                       \
   0x0b) /*!< NVS is in an inconsistent state due to a previous error. Call \
            nvs_flash_init and nvs_open again, then retry. */
#define ESP_ERR_NVS_INVALID_LENGTH \
  (ESP_ERR_NVS_BASE +              \
   0x0c) /*!< String or blob length is not sufficient to store data */
#define ESP_ERR_NVS_NO_FREE_PAGES                                              \
  (ESP_ERR_NVS_BASE +                                                          \
   0x0d) /*!< NVS partition doesn't contain any empty pages. This may happen   \
            if NVS partition was truncated. Erase the whole partition and call \
            nvs_flash_init again. */
#define ESP_ERR_NVS_VALUE_TOO_LONG                                    \
  (ESP_ERR_NVS_BASE + 0x0e) /*!< String or blob length is longer than \
                               supported by the implementation */
#define ESP_ERR_NVS_PART_NOT_FOUND                                             \
  (ESP_ERR_NVS_BASE + 0x0f) /*!< Partition with specified name is not found in \
                               the partition table */

#define ESP_ERR_NVS_NEW_VERSION_FOUND                                          \
  (ESP_ERR_NVS_BASE + 0x10) /*!< NVS partition contains data in new format and \
                               cannot be recognized by this version of code */
#define ESP_ERR_NVS_XTS_ENCR_FAILED \
  (ESP_ERR_NVS_BASE +               \
   0x11) /*!< XTS encryption failed while writing NVS entry */
#define ESP_ERR_NVS_XTS_DECR_FAILED \
  (ESP_ERR_NVS_BASE +               \
   0x12) /*!< XTS decryption failed while reading NVS entry */
#define ESP_ERR_NVS_XTS_CFG_FAILED \
  (ESP_ERR_NVS_BASE + 0x13) /*!< XTS configuration setting failed */
#define ESP_ERR_NVS_XTS_CFG_NOT_FOUND \
  (ESP_ERR_NVS_BASE + 0x14) /*!< XTS configuration not found */
#define ESP_ERR_NVS_ENCR_NOT_SUPPORTED \
  (ESP_ERR_NVS_BASE +                  \
   0x15) /*!< NVS encryption is not supported in this version */
#define ESP_ERR_NVS_KEYS_NOT_INITIALIZED \
  (ESP_ERR_NVS_BASE + 0x16) /*!< NVS key partition is uninitialized */
#define ESP_ERR_NVS_CORRUPT_KEY_PART \
  (ESP_ERR_NVS_BASE + 0x17) /*!< NVS key partition is corrupt */
#define ESP_ERR_NVS_WRONG_ENCRYPTION                                       \
  (ESP_ERR_NVS_BASE + 0x19) /*!< NVS partition is marked as encrypted with \
                               generic flash encryption. This is forbidden \
                               since the NVS encryption works differently. */

#define ESP_ERR_NVS_CONTENT_DIFFERS                                            \
  (ESP_ERR_NVS_BASE +                                                          \
   0x18) /*!< Internal error; never returned by nvs API functions.  NVS key is \
            different in comparison */

#define NVS_DEFAULT_PART_NAME                                             \
  "nvs" /*!< Default partition name of the NVS partition in the partition \
           table */

class NvsImpl {
 public:
  struct Handle {
    std::string partition_name;
    std::string ns_name;
    bool readonly;
  };

  NvsImpl(const std::string& path) : path_(path), next_id_(0) {
    if (!path_.empty()) {
      std::ifstream input_file(path);
      if (!input_file.is_open()) {
        std::cerr << "WARNING: could not open the NVS storage file - '" << path
                  << "'. Creating new one." << std::endl;
        // Try creating a new file.
        save();
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

  esp_err_t init(const char* partition_name) {
    (*storage_.mutable_partitions())[partition_name];
    save();
    return ESP_OK;
  }

  void save() {
    std::string val;
    if (!TextFormat::PrintToString(storage_, &val)) {
      std::cerr << "Could not print" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::ofstream output_file(path_);
    output_file << val;
    output_file.close();
  }

  esp_err_t open(const char* part_name, const char* name, bool readonly,
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

  esp_err_t set(nvs_handle_t handle, const char* key,
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

  esp_err_t get(nvs_handle_t handle, const char* key,
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

  void close(nvs_handle_t handle) { open_partitions_.erase(handle); }

  esp_err_t erase_key(nvs_handle_t handle, const char* key) {
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

  esp_err_t erase_all(nvs_handle_t handle) {
    if (open_partitions_.find(handle) == open_partitions_.end()) {
      return ESP_ERR_NVS_INVALID_HANDLE;
    }
    Handle& h = open_partitions_[handle];
    if (h.readonly) {
      return ESP_ERR_NVS_READ_ONLY;
    }
    (*(*storage_.mutable_partitions())[h.partition_name]
          .mutable_name_spaces())[h.ns_name]
        .mutable_entries()
        ->clear();
    return ESP_OK;
  }

 private:
  roo_testing::esp32::nvs::Nvs storage_;
  std::string path_;
  std::map<int, Handle> open_partitions_;
  int next_id_;
};

Nvs::Nvs(const std::string& path) : impl_(new NvsImpl(path)) {}

Nvs::~Nvs() { delete impl_; }

void Nvs::save() { impl_->save(); }

esp_err_t Nvs::init(const char* partition_name) {
  return impl_->init(partition_name);
}

esp_err_t Nvs::open(const char* part_name, const char* name, bool readonly,
                    nvs_handle_t* out_handle) {
  return impl_->open(part_name, name, readonly, out_handle);
}

esp_err_t Nvs::set_i8(nvs_handle_t handle, const char* key, int8_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I8);
  val.set_sint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U8);
  val.set_uint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i16(nvs_handle_t handle, const char* key, int16_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I16);
  val.set_sint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U16);
  val.set_uint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i32(nvs_handle_t handle, const char* key, int32_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I32);
  val.set_sint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U32);
  val.set_uint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_i64(nvs_handle_t handle, const char* key, int64_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::I64);
  val.set_sint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_u64(nvs_handle_t handle, const char* key, uint64_t value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::U64);
  val.set_uint_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_str(nvs_handle_t handle, const char* key,
                       const char* value) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::STR);
  val.set_str_value(value);
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::set_blob(nvs_handle_t handle, const char* key, const void* value,
                        size_t length) {
  roo_testing::esp32::nvs::EntryValue val;
  val.set_type(roo_testing::esp32::nvs::BLOB);
  val.set_blob_value(std::string((const char*)value, length));
  return impl_->set(handle, key, std::move(val));
}

esp_err_t Nvs::get_i8(nvs_handle_t handle, const char* key, int8_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::I8, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u8(nvs_handle_t handle, const char* key, uint8_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::U8, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i16(nvs_handle_t handle, const char* key, int16_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::I16, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u16(nvs_handle_t handle, const char* key, uint16_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::U16, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i32(nvs_handle_t handle, const char* key, int32_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::I32, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u32(nvs_handle_t handle, const char* key, uint32_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::U32, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_i64(nvs_handle_t handle, const char* key, int64_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::I64, val);
  if (err != ESP_OK) return err;
  *value = val.sint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_u64(nvs_handle_t handle, const char* key, uint64_t* value) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::U64, val);
  if (err != ESP_OK) return err;
  *value = val.uint_value();
  return ESP_OK;
}

esp_err_t Nvs::get_str(nvs_handle_t handle, const char* key, char* value,
                       size_t* length) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::STR, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.str_value().size() + 1);
  if (value != nullptr) {
    memcpy(value, val.str_value().c_str(), actual);
  }
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::get_blob(nvs_handle_t handle, const char* key, char* value,
                        size_t* length) {
  roo_testing::esp32::nvs::EntryValue val;
  esp_err_t err = impl_->get(handle, key, roo_testing::esp32::nvs::BLOB, val);
  if (err != ESP_OK) return err;
  size_t actual = std::max(*length, val.blob_value().size());
  if (value != nullptr) {
    memcpy(value, val.blob_value().c_str(), actual);
  }
  *length = actual;
  return ESP_OK;
}

esp_err_t Nvs::commit() {
  impl_->save();
  return ESP_OK;
}

void Nvs::close(nvs_handle_t handle) { impl_->close(handle); }

esp_err_t Nvs::erase_key(nvs_handle_t handle, const char* key) {
  return impl_->erase_key(handle, key);
}

esp_err_t Nvs::erase_all(nvs_handle_t handle) {
  return impl_->erase_all(handle);
}
