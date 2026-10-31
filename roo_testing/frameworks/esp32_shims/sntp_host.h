#pragma once
#include <stdint.h>

#include "esp_err.h"

namespace roo_testing::esp32::sntp {
/// Chooses real host UTC or an explicitly anchored server timeline.
enum class TimeSource { kHost, kReference };

/// Configures one virtual server behavior for the configured SNTP server slots.
/// Reference time is anchored when ConfigureServer succeeds; delays use uptime.
struct ServerBehavior {
  TimeSource time_source = TimeSource::kHost;
  int64_t reference_unix_us = 0;
  int64_t response_delay_us = 100000;
  bool responds = true;
};

/// Configures a stopped, quiescent client from task context, without setting
/// UTC. Invalid input returns INVALID_ARG; an active/resetting client
/// INVALID_STATE. Failure preserves the previous behavior. No actual network
/// traffic is used.
esp_err_t ConfigureServer(const ServerBehavior& behavior);

/// Stops and drains delivery, restoring defaults without changing device UTC.
/// Task-context test isolation barrier requiring exclusive ownership. Must not
/// be called from a notification/override. Already-claimed delivery may finish.
void Reset();
}  // namespace roo_testing::esp32::sntp

namespace roo_testing::esp32_shims {
/// Initializes SNTP task synchronization before application tasks start using
/// it.
void InitializeSntpForHost();

/// Drains SNTP and releases its timer before ESP timer service shutdown.
/// Called from the runner task, outside notification context.
esp_err_t ShutdownSntpForHost();
}  // namespace roo_testing::esp32_shims
