#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Host configuration for an ESP-IDF LittleFS mount.
typedef struct {
  const char* base_path;
  const char* partition_label;
  const esp_partition_t* partition;
  bool format_if_mount_failed;
  bool read_only;
  bool dont_mount;
  bool grow_on_mount;
} esp_vfs_littlefs_conf_t;

/// Registers a host directory as a LittleFS partition.
esp_err_t esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t* conf);

/// Unregisters a previously mounted partition.
esp_err_t esp_vfs_littlefs_unregister(const char* partition_label);

/// Reports whether the partition is currently registered.
bool esp_littlefs_mounted(const char* partition_label);

/// Clears the contents of the registered or default host directory.
esp_err_t esp_littlefs_format(const char* partition_label);

/// Reports host directory usage; total capacity is not simulated.
esp_err_t esp_littlefs_info(const char* partition_label, size_t* total_bytes,
                            size_t* used_bytes);

#ifdef __cplusplus
}
#endif
