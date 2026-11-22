#pragma once

#include <cstdint>

namespace roo_testing::interrupt_delivery {

/// Delivers pending interrupts at a bare-metal Pico runtime checkpoint.
void DispatchPicoInterrupts();

/// Marks execution of the given non-nested Pico timer interrupt.
void EnterPicoTimerInterrupt(unsigned alarm_num);

/// Resumes ordinary Pico core execution after a timer interrupt.
void LeavePicoTimerInterrupt();

}  // namespace roo_testing::interrupt_delivery
