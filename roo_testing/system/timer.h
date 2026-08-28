#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <functional>

using SystemTimeAlarmId = uint64_t;

/// Schedules `callback` to run at the absolute emulated uptime deadline.
SystemTimeAlarmId ScheduleSystemTimeAlarm(int64_t deadline_uptime_us,
                                          std::function<void()> callback);

/// Cancels an unclaimed system-time alarm, if it is still pending.
void CancelSystemTimeAlarm(SystemTimeAlarmId id);

/// Delivers all alarms due at the current emulated uptime.
void ProcessSystemTimeAlarms();

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

/// Occupies host time without advancing emulated uptime or dispatching work.
void system_time_busy_wait_micros(uint64_t us);

#ifdef __cplusplus
}
#endif
