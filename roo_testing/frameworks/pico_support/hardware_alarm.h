#pragma once

#include <cstdint>

namespace roo_testing::pico {

/// Identifies the absence of a hardware compare deadline.
inline constexpr uint64_t kNoHardwareAlarm = UINT64_MAX;

/// Latches due compares and delivers pending interrupts on the emulated core.
void PollTimerInterrupts();

/// Returns the nearest armed compare, or kNoHardwareAlarm.
uint64_t NextHardwareAlarm();

}  // namespace roo_testing::pico
