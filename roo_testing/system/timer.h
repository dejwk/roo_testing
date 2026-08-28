#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Synchronizes emulated uptime with host time when auto-sync is enabled.
void system_time_sync();

/// Advances emulated uptime by the supplied nanoseconds.
void system_time_lag_ns(uint64_t ns);

/// Returns the current emulated uptime in microseconds.
int64_t system_time_get_micros();

/// Reports whether this process selected host-following emulated time.
bool system_time_is_auto_sync_enabled();

/// Advances emulated uptime by the supplied microseconds.
void system_time_delay_micros(uint64_t us);

/// Changes the emulated-time mode. Deprecated; select manual mode at link time.
void system_time_set_auto_sync(bool auto_sync)
    __attribute__((deprecated("Select //roo_testing/system:manual_time_mode "
                              "instead.")));

#ifdef __cplusplus
}
#endif
