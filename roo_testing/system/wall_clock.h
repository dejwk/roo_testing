#pragma once
#include <stdint.h>

namespace roo_testing::system {
/// Sets process-local UTC at the current emulated uptime without changing
/// uptime. Returns false without mutation if the current clock cannot be
/// represented.
bool SetUnixTimeMicros(int64_t unix_us);

/// Reads UTC, initially epoch plus emulated uptime. Leaves output on overflow.
/// Safe from host threads and tasks, not ISR/signal context; does not dispatch.
bool GetUnixTimeMicros(int64_t& unix_us);

/// Clears the UTC correction, restoring epoch plus this boot's current uptime.
/// Does not restart uptime, read host time, or change SNTP state.
void ResetToPowerOnMapping();
}  // namespace roo_testing::system
