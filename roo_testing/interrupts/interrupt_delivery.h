#pragma once

#include <cstdint>

namespace roo_testing::interrupt_delivery {

/// Reports whether the current context can change handler registrations.
bool CanRegister();

/// Connects the runtime to the controller's pending-interrupt dispatcher.
bool InstallDispatcher(void (*dispatch)());

/// Masks dispatch while the controller publishes a registration change.
uint32_t Mask();

/// Restores the dispatch state saved by Mask().
void Restore(uint32_t state);

/// Requests delivery of pending interrupts through the selected runtime.
void RequestDispatch() noexcept;

}  // namespace roo_testing::interrupt_delivery
