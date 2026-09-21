#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

#include <vector>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

constexpr char kDefaultPartition[] = "nvs";

Nvs& Storage() { return FakeEsp32().nvs; }

nvs_sec_scheme_t* security_scheme = nullptr;

}  // namespace

struct nvs_opaque_iterator_t {
  std::vector<Nvs::EntryInfo> entries;
  size_t index = 0;
};

extern "C" {

esp_err_t nvs_flash_init(void) { return Storage().init(kDefaultPartition); }

esp_err_t nvs_flash_init_partition(const char* partition_label) {
  if (partition_label == nullptr) return ESP_ERR_INVALID_ARG;
  return Storage().init(partition_label);
}

esp_err_t nvs_flash_init_partition_ptr(const esp_partition_t* partition) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  return Storage().init(partition->label, partition->size);
}

esp_err_t nvs_flash_init_partition_bdl(const char* partition_label,
                                       esp_blockdev_handle_t bdl) {
  if (partition_label == nullptr || bdl == nullptr) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t nvs_flash_deinit(void) { return Storage().deinit(kDefaultPartition); }
esp_err_t nvs_flash_deinit_partition(const char* partition_label) {
  return Storage().deinit(partition_label);
}
esp_err_t nvs_flash_erase(void) {
  return Storage().erase_partition(kDefaultPartition);
}
esp_err_t nvs_flash_erase_partition(const char* partition_label) {
  return Storage().erase_partition(partition_label);
}
esp_err_t nvs_flash_erase_partition_ptr(const esp_partition_t* partition) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  return Storage().erase_partition(partition->label);
}
esp_err_t nvs_flash_secure_init(nvs_sec_cfg_t* cfg) {
  return cfg == nullptr ? nvs_flash_init() : ESP_ERR_NVS_ENCR_NOT_SUPPORTED;
}
esp_err_t nvs_flash_secure_init_partition(const char* partition,
                                          nvs_sec_cfg_t* cfg) {
  if (partition == nullptr) return ESP_ERR_INVALID_ARG;
  return cfg == nullptr ? nvs_flash_init_partition(partition)
                        : ESP_ERR_NVS_ENCR_NOT_SUPPORTED;
}

esp_err_t nvs_flash_generate_keys(const esp_partition_t* partition,
                                  nvs_sec_cfg_t* cfg) {
  if (partition == nullptr || cfg == nullptr) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_NVS_ENCR_NOT_SUPPORTED;
}

esp_err_t nvs_flash_read_security_cfg(const esp_partition_t* partition,
                                      nvs_sec_cfg_t* cfg) {
  if (partition == nullptr || cfg == nullptr) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_NVS_ENCR_NOT_SUPPORTED;
}

esp_err_t nvs_flash_register_security_scheme(nvs_sec_scheme_t* scheme_cfg) {
  if (scheme_cfg == nullptr) return ESP_ERR_INVALID_ARG;
  security_scheme = scheme_cfg;
  return ESP_OK;
}

void nvs_flash_deregister_security_scheme(void) { security_scheme = nullptr; }

nvs_sec_scheme_t* nvs_flash_get_default_security_scheme(void) {
  return security_scheme;
}

esp_err_t nvs_flash_generate_keys_v2(nvs_sec_scheme_t* scheme_cfg,
                                     nvs_sec_cfg_t* cfg) {
  if (scheme_cfg == nullptr || cfg == nullptr) return ESP_ERR_INVALID_ARG;
  if (scheme_cfg->nvs_flash_key_gen == nullptr) return ESP_ERR_NOT_SUPPORTED;
  return scheme_cfg->nvs_flash_key_gen(scheme_cfg->scheme_data, cfg);
}

esp_err_t nvs_flash_read_security_cfg_v2(nvs_sec_scheme_t* scheme_cfg,
                                         nvs_sec_cfg_t* cfg) {
  if (scheme_cfg == nullptr || cfg == nullptr) return ESP_ERR_INVALID_ARG;
  if (scheme_cfg->nvs_flash_read_cfg == nullptr) return ESP_ERR_NOT_SUPPORTED;
  return scheme_cfg->nvs_flash_read_cfg(scheme_cfg->scheme_data, cfg);
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

esp_err_t nvs_find_key(nvs_handle_t h, const char* k, nvs_type_t* out_type) {
  Nvs::Type type;
  esp_err_t result = Storage().find_key(h, k, out_type ? &type : nullptr);
  if (result == ESP_OK && out_type != nullptr) {
    *out_type = static_cast<nvs_type_t>(type);
  }
  return result;
}

esp_err_t nvs_entry_find(const char* part_name, const char* namespace_name,
                         nvs_type_t type, nvs_iterator_t* output_iterator) {
  if (part_name == nullptr || output_iterator == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  auto* iterator = new nvs_opaque_iterator_t;
  esp_err_t result = Storage().list_entries(
      part_name, namespace_name, static_cast<int>(type), &iterator->entries);
  if (result != ESP_OK) {
    delete iterator;
    *output_iterator = nullptr;
    return result;
  }
  *output_iterator = iterator;
  return ESP_OK;
}

esp_err_t nvs_entry_find_in_handle(nvs_handle_t handle, nvs_type_t type,
                                   nvs_iterator_t* output_iterator) {
  if (output_iterator == nullptr) return ESP_ERR_INVALID_ARG;
  auto* iterator = new nvs_opaque_iterator_t;
  esp_err_t result = Storage().list_entries(
      handle, static_cast<int>(type), &iterator->entries);
  if (result != ESP_OK) {
    delete iterator;
    *output_iterator = nullptr;
    return result;
  }
  *output_iterator = iterator;
  return ESP_OK;
}

esp_err_t nvs_entry_next(nvs_iterator_t* iterator) {
  if (iterator == nullptr || *iterator == nullptr) return ESP_ERR_INVALID_ARG;
  if (++(*iterator)->index < (*iterator)->entries.size()) return ESP_OK;
  delete *iterator;
  *iterator = nullptr;
  return ESP_ERR_NVS_NOT_FOUND;
}

esp_err_t nvs_entry_info(const nvs_iterator_t iterator,
                         nvs_entry_info_t* out_info) {
  if (iterator == nullptr || out_info == nullptr) return ESP_ERR_INVALID_ARG;
  if (iterator->index >= iterator->entries.size()) return ESP_ERR_INVALID_ARG;
  const Nvs::EntryInfo& entry = iterator->entries[iterator->index];
  memset(out_info, 0, sizeof(*out_info));
  strncpy(out_info->namespace_name, entry.namespace_name.c_str(),
          sizeof(out_info->namespace_name) - 1);
  strncpy(out_info->key, entry.key.c_str(), sizeof(out_info->key) - 1);
  out_info->type = static_cast<nvs_type_t>(entry.type);
  return ESP_OK;
}

void nvs_release_iterator(nvs_iterator_t iterator) { delete iterator; }

esp_err_t nvs_erase_key(nvs_handle_t h, const char* k) {
  return Storage().erase_key(h, k);
}
esp_err_t nvs_erase_all(nvs_handle_t h) { return Storage().erase_all(h); }
esp_err_t nvs_purge_all(nvs_handle_t h) { return Storage().purge_all(h); }
esp_err_t nvs_commit(nvs_handle_t h) { return Storage().commit(h); }
void nvs_close(nvs_handle_t h) { Storage().close(h); }

esp_err_t nvs_get_stats(const char* partition_name, nvs_stats_t* stats) {
  if (stats == nullptr) return ESP_ERR_INVALID_ARG;
  Nvs::Stats result;
  esp_err_t status = Storage().get_stats(
      partition_name == nullptr ? kDefaultPartition : partition_name, &result);
  if (status != ESP_OK) {
    memset(stats, 0, sizeof(*stats));
    return status;
  }
  stats->used_entries = result.used_entries;
  stats->free_entries = result.free_entries;
  stats->available_entries = result.available_entries;
  stats->total_entries = result.total_entries;
  stats->namespace_count = result.namespace_count;
  return ESP_OK;
}

esp_err_t nvs_get_used_entry_count(nvs_handle_t handle, size_t* used_entries) {
  return Storage().get_used_entry_count(handle, used_entries);
}

}  // extern "C"
