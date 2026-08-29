#pragma once

#include "esp_err.h"

namespace roo_testing::esp32_shims {

/// Initializes the vendored ESP timer service from a scheduled host task.
esp_err_t InitializeEspTimerForHost();

/// Stops the timer service before the host generic alarm service closes.
esp_err_t ShutdownEspTimerForHost();

}  // namespace roo_testing::esp32_shims
