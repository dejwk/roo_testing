#include "esp_ota_ops.h"
#include "esp_partition.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace {

constexpr size_t kPartitionSize = 4U * 1024U * 1024U;

esp_partition_t g_running_partition = {
    .flash_chip = nullptr,
    .type = ESP_PARTITION_TYPE_APP,
    .subtype = ESP_PARTITION_SUBTYPE_APP_FACTORY,
    .address = 0x10000,
    .size = kPartitionSize,
    .erase_size = 4096,
    .label = "host_app",
    .encrypted = false,
    .readonly = false,
};

esp_partition_t g_update_partition = {
    .flash_chip = nullptr,
    .type = ESP_PARTITION_TYPE_APP,
    .subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0,
    .address = 0x410000,
    .size = kPartitionSize,
    .erase_size = 4096,
    .label = "host_ota",
    .encrypted = false,
    .readonly = false,
};

const esp_partition_t* g_boot_partition = &g_running_partition;
std::vector<uint8_t> g_running_data(kPartitionSize, 0xFF);
std::vector<uint8_t> g_update_data(kPartitionSize, 0xFF);

struct Iterator {
  const esp_partition_t* partition;
};

std::vector<uint8_t>* DataFor(const esp_partition_t* partition) {
  if (partition == &g_running_partition) return &g_running_data;
  if (partition == &g_update_partition) return &g_update_data;
  return nullptr;
}

bool Matches(const esp_partition_t& partition, esp_partition_type_t type,
             esp_partition_subtype_t subtype, const char* label) {
  return (type == ESP_PARTITION_TYPE_ANY || partition.type == type) &&
         (subtype == ESP_PARTITION_SUBTYPE_ANY ||
          partition.subtype == subtype) &&
         (label == nullptr || strcmp(partition.label, label) == 0);
}

const esp_partition_t* Find(esp_partition_type_t type,
                            esp_partition_subtype_t subtype,
                            const char* label) {
  if (Matches(g_running_partition, type, subtype, label)) {
    return &g_running_partition;
  }
  if (Matches(g_update_partition, type, subtype, label)) {
    return &g_update_partition;
  }
  return nullptr;
}

esp_err_t ValidateRange(const esp_partition_t* partition, size_t offset,
                        size_t size) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  if (offset > partition->size || size > partition->size - offset) {
    return ESP_ERR_INVALID_SIZE;
  }
  return ESP_OK;
}

}  // namespace

extern "C" {

esp_partition_iterator_t esp_partition_find(esp_partition_type_t type,
                                            esp_partition_subtype_t subtype,
                                            const char* label) {
  const esp_partition_t* partition = Find(type, subtype, label);
  if (partition == nullptr) return nullptr;
  return reinterpret_cast<esp_partition_iterator_t>(new Iterator{partition});
}

const esp_partition_t* esp_partition_find_first(
    esp_partition_type_t type, esp_partition_subtype_t subtype,
    const char* label) {
  return Find(type, subtype, label);
}

const esp_partition_t* esp_partition_get(esp_partition_iterator_t iterator) {
  if (iterator == nullptr) return nullptr;
  return reinterpret_cast<Iterator*>(iterator)->partition;
}

esp_partition_iterator_t esp_partition_next(
    esp_partition_iterator_t iterator) {
  if (iterator == nullptr) return nullptr;
  auto* host = reinterpret_cast<Iterator*>(iterator);
  if (host->partition == &g_running_partition) {
    host->partition = &g_update_partition;
    return iterator;
  }
  delete host;
  return nullptr;
}

void esp_partition_iterator_release(esp_partition_iterator_t iterator) {
  delete reinterpret_cast<Iterator*>(iterator);
}

const esp_partition_t* esp_partition_verify(const esp_partition_t* partition) {
  if (partition == nullptr) return nullptr;
  for (const auto* known : {&g_running_partition, &g_update_partition}) {
    if (known == partition ||
        (known->address == partition->address && known->size == partition->size &&
         known->type == partition->type && known->subtype == partition->subtype &&
         strcmp(known->label, partition->label) == 0)) {
      return known;
    }
  }
  return nullptr;
}

esp_err_t esp_partition_read(const esp_partition_t* partition, size_t offset,
                             void* destination, size_t size) {
  const esp_err_t valid = ValidateRange(partition, offset, size);
  if (valid != ESP_OK || (destination == nullptr && size != 0)) {
    return valid == ESP_OK ? ESP_ERR_INVALID_ARG : valid;
  }
  auto* data = DataFor(partition);
  if (data == nullptr) {
    memset(destination, 0xFF, size);
  } else {
    memcpy(destination, data->data() + offset, size);
  }
  return ESP_OK;
}

esp_err_t esp_partition_write(const esp_partition_t* partition, size_t offset,
                              const void* source, size_t size) {
  const esp_err_t valid = ValidateRange(partition, offset, size);
  if (valid != ESP_OK || (source == nullptr && size != 0)) {
    return valid == ESP_OK ? ESP_ERR_INVALID_ARG : valid;
  }
  if (partition->readonly) return ESP_ERR_NOT_ALLOWED;
  auto* data = DataFor(partition);
  if (data != nullptr) memcpy(data->data() + offset, source, size);
  return ESP_OK;
}

esp_err_t esp_partition_read_raw(const esp_partition_t* partition,
                                 size_t offset, void* destination,
                                 size_t size) {
  return esp_partition_read(partition, offset, destination, size);
}

esp_err_t esp_partition_write_raw(const esp_partition_t* partition,
                                  size_t offset, const void* source,
                                  size_t size) {
  return esp_partition_write(partition, offset, source, size);
}

esp_err_t esp_partition_erase_range(const esp_partition_t* partition,
                                    size_t offset, size_t size) {
  const esp_err_t valid = ValidateRange(partition, offset, size);
  if (valid != ESP_OK) return valid;
  if (partition->readonly) return ESP_ERR_NOT_ALLOWED;
  auto* data = DataFor(partition);
  if (data != nullptr) std::fill_n(data->begin() + offset, size, 0xFF);
  return ESP_OK;
}

esp_err_t esp_partition_mmap(const esp_partition_t* partition, size_t offset,
                             size_t size, esp_partition_mmap_memory_t,
                             const void** out_ptr,
                             esp_partition_mmap_handle_t* out_handle) {
  const esp_err_t valid = ValidateRange(partition, offset, size);
  auto* data = DataFor(partition);
  if (valid != ESP_OK || data == nullptr || out_ptr == nullptr ||
      out_handle == nullptr) {
    return valid == ESP_OK ? ESP_ERR_INVALID_ARG : valid;
  }
  *out_ptr = data->data() + offset;
  *out_handle = static_cast<esp_partition_mmap_handle_t>(1);
  return ESP_OK;
}

void esp_partition_munmap(esp_partition_mmap_handle_t) {}

esp_err_t esp_partition_get_sha256(const esp_partition_t* partition,
                                   uint8_t* sha) {
  if (partition == nullptr || sha == nullptr) return ESP_ERR_INVALID_ARG;
  memset(sha, 0, 32);
  return ESP_OK;
}

bool esp_partition_check_identity(const esp_partition_t* first,
                                  const esp_partition_t* second) {
  return first == second;
}

int esp_ota_get_app_elf_sha256(char* destination, size_t size) {
  if (destination != nullptr && size != 0) destination[0] = '\0';
  return 0;
}

esp_err_t esp_ota_begin(const esp_partition_t* partition, size_t,
                        esp_ota_handle_t* out_handle) {
  if (partition == nullptr || out_handle == nullptr) return ESP_ERR_INVALID_ARG;
  *out_handle = static_cast<esp_ota_handle_t>(partition->address);
  return ESP_OK;
}
esp_err_t esp_ota_resume(const esp_partition_t* partition, size_t, size_t,
                         esp_ota_handle_t* out_handle) {
  return esp_ota_begin(partition, 0, out_handle);
}
esp_err_t esp_ota_set_final_partition(esp_ota_handle_t,
                                      const esp_partition_t*, bool) {
  return ESP_OK;
}
esp_err_t esp_ota_write(esp_ota_handle_t, const void*, size_t) {
  return ESP_OK;
}
esp_err_t esp_ota_write_with_offset(esp_ota_handle_t, const void*, size_t,
                                    uint32_t) {
  return ESP_OK;
}
esp_err_t esp_ota_end(esp_ota_handle_t) { return ESP_OK; }
esp_err_t esp_ota_abort(esp_ota_handle_t) { return ESP_OK; }
esp_err_t esp_ota_set_boot_partition(const esp_partition_t* partition) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  g_boot_partition = partition;
  return ESP_OK;
}
const esp_partition_t* esp_ota_get_boot_partition(void) {
  return g_boot_partition;
}
const esp_partition_t* esp_ota_get_running_partition(void) {
  return &g_running_partition;
}
const esp_partition_t* esp_ota_get_next_update_partition(
    const esp_partition_t*) {
  return &g_update_partition;
}
esp_err_t esp_ota_get_partition_description(const esp_partition_t* partition,
                                            esp_app_desc_t* description) {
  if (partition == nullptr || description == nullptr) return ESP_ERR_INVALID_ARG;
  memset(description, 0, sizeof(*description));
  return ESP_OK;
}
uint8_t esp_ota_get_app_partition_count(void) { return 1; }
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) { return ESP_OK; }
esp_err_t esp_ota_mark_app_invalid_rollback(void) { return ESP_OK; }
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void) { return ESP_OK; }
const esp_partition_t* esp_ota_get_last_invalid_partition(void) {
  return nullptr;
}
esp_err_t esp_ota_get_state_partition(const esp_partition_t*,
                                      esp_ota_img_states_t*) {
  return ESP_ERR_NOT_FOUND;
}
esp_err_t esp_ota_erase_last_boot_app_partition(void) { return ESP_OK; }
bool esp_ota_check_rollback_is_possible(void) { return false; }
esp_err_t esp_ota_invalidate_inactive_ota_data_slot(void) { return ESP_OK; }

}  // extern "C"
