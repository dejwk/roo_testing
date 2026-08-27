#pragma once

#include <cstdint>

namespace roo_testing {

/// Raw handler invoked in emulated FreeRTOS ISR context.
using InterruptHandler = void (*)(void* argument);

/// Identifies one live interrupt-handler registration.
struct InterruptHandle {
  uint32_t slot = static_cast<uint32_t>(-1);
  uint32_t generation = 0;

  /// Returns true when this value can identify a registration.
  constexpr bool isValid() const noexcept {
    return slot != static_cast<uint32_t>(-1) && generation != 0;
  }
};

/// Outcome of registering an interrupt handler.
enum class InterruptRegistrationResult {
  kRegistered,
  kInvalidArgument,
  kWrongContext,
  kNoCapacity,
  kBackendUnavailable,
};

/// Registers a handler from a scheduler-running FreeRTOS task.
///
/// The handler and argument must remain valid until successful unregistration.
/// The handler runs in POSIX-signal and FreeRTOS ISR context, so it must use
/// only ISR-safe APIs and must not throw, block, lock, or allocate.
InterruptRegistrationResult registerInterrupt(InterruptHandler handler,
                                              void* argument,
                                              bool initially_enabled,
                                              InterruptHandle* out_handle);

/// Disables and removes a registration from a FreeRTOS task.
///
/// A successful return is quiescent on the current single-dispatch-context
/// backend: the handler is no longer executing. Any future parallel backend
/// must provide equivalent in-flight quiescence before returning success.
bool unregisterInterrupt(InterruptHandle handle);

/// Enables a registration and dispatches any pending request.
///
/// This lock-free operation may be called from a native host thread or an
/// emulated ISR.
bool enableInterrupt(InterruptHandle handle) noexcept;

/// Disables a registration without discarding a pending request.
///
/// This lock-free operation may be called from a native host thread or an
/// emulated ISR.
bool disableInterrupt(InterruptHandle handle) noexcept;

/// Coalesces one pending delivery for a registration.
///
/// This lock-free operation may be called from a native host thread or an
/// emulated ISR. Returns false when the handle no longer names a registration.
bool setInterruptPending(InterruptHandle handle) noexcept;

}  // namespace roo_testing
