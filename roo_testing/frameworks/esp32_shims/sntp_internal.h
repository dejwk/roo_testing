#pragma once
#include <stdint.h>

#include "esp_sntp.h"

namespace roo_testing::esp32_shims {
/// Registers a notification and returns its process-unique registration number.
uint64_t InstallSntpNotificationForHost(sntp_sync_time_cb_t callback);

/// Identifies the registration captured for this thread's current notification.
/// Zero outside notification delivery; nested delivery restores its
/// predecessor.
uint64_t CurrentSntpNotificationForHost();

/// Initializes the esp-netif adapter mutex before application tasks use it.
void InitializeNetifSntpForHost();
}  // namespace roo_testing::esp32_shims
