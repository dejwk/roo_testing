#pragma once

#include "esp_wifi_types.h"

namespace roo_testing {
namespace esp32 {
namespace wifi {

enum class DriverState {
  kUninitialized,
  kStopped,
  kStarted,
};

enum class StationState {
  kDisabled,
  kIdle,
  kScanning,
  kConnecting,
  kAssociated,
  kGotIp,
};

struct StateSnapshot {
  DriverState driver;
  StationState station;
  wifi_mode_t mode;
};

// Restores power-on state and cancels pending host WiFi work. Intended for
// deterministic test isolation; normal firmware should use the ESP-IDF API.
void Reset();

StateSnapshot GetState();

}  // namespace wifi
}  // namespace esp32
}  // namespace roo_testing
