#include <sys/stat.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"

namespace {

struct HostSdMount {
  sdmmc_card_t card = {};
  std::string base_path;
};

std::mutex& HostSdMutex() {
  static auto* mutex = new std::mutex();
  return *mutex;
}

std::map<sdmmc_card_t*, std::unique_ptr<HostSdMount>>& HostSdMounts() {
  static auto* mounts =
      new std::map<sdmmc_card_t*, std::unique_ptr<HostSdMount>>();
  return *mounts;
}

bool IsDirectory(const char* path) {
  struct stat status;
  return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

esp_err_t MountHostSd(const char* base_path, const sdmmc_host_t* host_config,
                      const void* slot_config,
                      const esp_vfs_fat_mount_config_t* mount_config,
                      sdmmc_card_t** out_card) {
  if (base_path == nullptr || host_config == nullptr ||
      slot_config == nullptr || mount_config == nullptr ||
      out_card == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *out_card = nullptr;
  if (!IsDirectory(base_path)) {
    return ESP_FAIL;
  }

  std::lock_guard<std::mutex> lock(HostSdMutex());
  for (const auto& entry : HostSdMounts()) {
    if (entry.second->base_path == base_path) {
      return ESP_ERR_INVALID_STATE;
    }
  }

  auto mount = std::make_unique<HostSdMount>();
  mount->base_path = base_path;
  sdmmc_card_t* card = &mount->card;
  HostSdMounts().emplace(card, std::move(mount));
  *out_card = card;
  return ESP_OK;
}

}  // namespace

extern "C" esp_err_t esp_vfs_fat_sdmmc_mount(
    const char* base_path, const sdmmc_host_t* host_config,
    const void* slot_config, const esp_vfs_fat_mount_config_t* mount_config,
    sdmmc_card_t** out_card) {
  return MountHostSd(base_path, host_config, slot_config, mount_config,
                     out_card);
}

extern "C" esp_err_t esp_vfs_fat_sdspi_mount(
    const char* base_path, const sdmmc_host_t* host_config,
    const sdspi_device_config_t* slot_config,
    const esp_vfs_fat_mount_config_t* mount_config, sdmmc_card_t** out_card) {
  return MountHostSd(base_path, host_config, slot_config, mount_config,
                     out_card);
}

extern "C" esp_err_t esp_vfs_fat_sdcard_unmount(const char* base_path,
                                                sdmmc_card_t* card) {
  if (base_path == nullptr || card == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }

  std::lock_guard<std::mutex> lock(HostSdMutex());
  auto& mounts = HostSdMounts();
  if (mounts.empty()) {
    return ESP_ERR_INVALID_STATE;
  }
  auto found = mounts.find(card);
  if (found == mounts.end() || found->second->base_path != base_path) {
    return ESP_ERR_INVALID_ARG;
  }
  mounts.erase(found);
  return ESP_OK;
}

// Host mounts operate directly on directories and never issue block-level
// transactions. ESP-IDF's default host descriptors still take the addresses
// of these routines, so keep those descriptors linkable and report unsupported
// operations when an application attempts actual card I/O.
extern "C" esp_err_t sdspi_host_init(void) { return ESP_OK; }

extern "C" esp_err_t sdspi_host_init_device(
    const sdspi_device_config_t* dev_config, sdspi_dev_handle_t* out_handle) {
  if (dev_config == nullptr || out_handle == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *out_handle = 1;
  return ESP_OK;
}

extern "C" esp_err_t sdspi_host_remove_device(sdspi_dev_handle_t handle) {
  (void)handle;
  return ESP_OK;
}

extern "C" esp_err_t sdspi_host_do_transaction(sdspi_dev_handle_t handle,
                                               sdmmc_command_t* command) {
  (void)handle;
  (void)command;
  return ESP_ERR_NOT_SUPPORTED;
}

extern "C" esp_err_t sdspi_host_set_card_clk(sdspi_dev_handle_t handle,
                                             uint32_t freq_khz) {
  (void)handle;
  (void)freq_khz;
  return ESP_OK;
}

extern "C" esp_err_t sdspi_host_get_real_freq(sdspi_dev_handle_t handle,
                                              int* real_freq_khz) {
  (void)handle;
  if (real_freq_khz == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *real_freq_khz = SDMMC_FREQ_DEFAULT;
  return ESP_OK;
}

extern "C" esp_err_t sdspi_host_deinit(void) { return ESP_OK; }

extern "C" esp_err_t sdspi_host_io_int_enable(sdspi_dev_handle_t handle) {
  (void)handle;
  return ESP_OK;
}

extern "C" esp_err_t sdspi_host_io_int_wait(sdspi_dev_handle_t handle,
                                            uint32_t timeout_ticks) {
  (void)handle;
  (void)timeout_ticks;
  return ESP_ERR_TIMEOUT;
}

extern "C" bool sdspi_host_check_buffer_alignment(int slot, const void* buffer,
                                                  size_t size) {
  (void)slot;
  (void)buffer;
  (void)size;
  return true;
}

extern "C" esp_err_t sdmmc_host_init(void) { return ESP_OK; }

extern "C" esp_err_t sdmmc_host_init_slot(
    int slot, const sdmmc_slot_config_t* slot_config) {
  if (slot_config == nullptr ||
      (slot != SDMMC_HOST_SLOT_0 && slot != SDMMC_HOST_SLOT_1)) {
    return ESP_ERR_INVALID_ARG;
  }
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_set_bus_width(int slot, size_t width) {
  (void)slot;
  (void)width;
  return ESP_OK;
}

extern "C" size_t sdmmc_host_get_slot_width(int slot) {
  (void)slot;
  return 4;
}

extern "C" esp_err_t sdmmc_host_set_card_clk(int slot, uint32_t freq_khz) {
  (void)slot;
  (void)freq_khz;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_set_bus_ddr_mode(int slot, bool ddr_enabled) {
  (void)slot;
  (void)ddr_enabled;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_set_cclk_always_on(int slot, bool always_on) {
  (void)slot;
  (void)always_on;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_do_transaction(int slot,
                                               sdmmc_command_t* command) {
  (void)slot;
  (void)command;
  return ESP_ERR_NOT_SUPPORTED;
}

extern "C" esp_err_t sdmmc_host_io_int_enable(int slot) {
  (void)slot;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_io_int_wait(int slot, uint32_t timeout_ticks) {
  (void)slot;
  (void)timeout_ticks;
  return ESP_ERR_TIMEOUT;
}

extern "C" esp_err_t sdmmc_host_deinit_slot(int slot) {
  (void)slot;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_deinit(void) { return ESP_OK; }

extern "C" esp_err_t sdmmc_host_get_real_freq(int slot, int* real_freq_khz) {
  (void)slot;
  if (real_freq_khz == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *real_freq_khz = SDMMC_FREQ_DEFAULT;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_set_input_delay(
    int slot, sdmmc_delay_phase_t delay_phase) {
  (void)slot;
  (void)delay_phase;
  return ESP_ERR_NOT_SUPPORTED;
}

extern "C" esp_err_t sdmmc_host_set_input_delayline(
    int slot, sdmmc_delay_line_t delay_line) {
  (void)slot;
  (void)delay_line;
  return ESP_ERR_NOT_SUPPORTED;
}

extern "C" bool sdmmc_host_check_buffer_alignment(int slot, const void* buffer,
                                                  size_t size) {
  (void)slot;
  (void)buffer;
  (void)size;
  return true;
}

extern "C" esp_err_t sdmmc_host_is_slot_set_to_uhs1(int slot, bool* is_uhs1) {
  (void)slot;
  if (is_uhs1 == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  *is_uhs1 = false;
  return ESP_OK;
}

extern "C" esp_err_t sdmmc_host_get_state(sdmmc_host_state_t* state) {
  if (state == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  state->host_initialized = true;
  state->num_of_init_slots = 1;
  return ESP_OK;
}
