#include "esp_littlefs.h"

#include <filesystem>
#include <map>
#include <mutex>
#include <string>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

struct MountRecord {
  std::string path;
};

std::mutex& MountMutex() {
  static std::mutex mutex;
  return mutex;
}

std::map<std::string, MountRecord>& Mounts() {
  static std::map<std::string, MountRecord> mounts;
  return mounts;
}

std::string Label(const char* label) { return label == nullptr ? "" : label; }

std::string DefaultPath() { return FakeEsp32().fs_root() + "/littlefs"; }

}  // namespace

extern "C" esp_err_t esp_vfs_littlefs_register(
    const esp_vfs_littlefs_conf_t* conf) {
  if (conf == nullptr || conf->base_path == nullptr ||
      conf->base_path[0] != '/') {
    return ESP_ERR_INVALID_ARG;
  }
  std::lock_guard<std::mutex> lock(MountMutex());
  const std::string label = Label(conf->partition_label);
  if (Mounts().count(label) != 0) return ESP_ERR_INVALID_STATE;

  const std::filesystem::path path(conf->base_path);
  std::error_code error;
  if (!std::filesystem::is_directory(path, error)) {
    if (!conf->format_if_mount_failed ||
        !std::filesystem::create_directories(path, error)) {
      return ESP_FAIL;
    }
  }
  Mounts().emplace(label, MountRecord{path.string()});
  return ESP_OK;
}

extern "C" esp_err_t esp_vfs_littlefs_unregister(const char* partition_label) {
  std::lock_guard<std::mutex> lock(MountMutex());
  return Mounts().erase(Label(partition_label)) == 0 ? ESP_ERR_INVALID_STATE
                                                     : ESP_OK;
}

extern "C" bool esp_littlefs_mounted(const char* partition_label) {
  std::lock_guard<std::mutex> lock(MountMutex());
  return Mounts().count(Label(partition_label)) != 0;
}

extern "C" esp_err_t esp_littlefs_format(const char* partition_label) {
  std::lock_guard<std::mutex> lock(MountMutex());
  const std::string label = Label(partition_label);
  const auto mounted = Mounts().find(label);
  if (mounted == Mounts().end() && !label.empty()) return ESP_ERR_NOT_FOUND;
  const std::filesystem::path path =
      mounted == Mounts().end() ? DefaultPath() : mounted->second.path;
  std::error_code error;
  if (!std::filesystem::exists(path, error)) {
    std::filesystem::create_directories(path, error);
    return error ? ESP_FAIL : ESP_OK;
  }
  if (!std::filesystem::is_directory(path, error)) return ESP_FAIL;
  for (std::filesystem::directory_iterator it(path, error), end;
       !error && it != end; it.increment(error)) {
    std::filesystem::remove_all(it->path(), error);
  }
  return error ? ESP_FAIL : ESP_OK;
}

extern "C" esp_err_t esp_littlefs_info(const char* partition_label,
                                       size_t* total_bytes,
                                       size_t* used_bytes) {
  if (total_bytes == nullptr || used_bytes == nullptr)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(MountMutex());
  const auto mounted = Mounts().find(Label(partition_label));
  if (mounted == Mounts().end()) return ESP_ERR_INVALID_STATE;
  *total_bytes = 0;
  *used_bytes = 0;
  return ESP_OK;
}
