#include "roo_testing/interrupts/pico_interrupt_delivery.h"

#include <atomic>

#include "roo_testing/interrupts/interrupt_delivery.h"

namespace {

constexpr unsigned kFirstExternalException = 16;

std::atomic<bool> dispatch_requested{false};
void (*dispatch_pending)() = nullptr;
bool interrupts_masked = false;
unsigned current_exception = 0;

}  // namespace

extern "C" unsigned roo_testing_pico_current_exception() {
  return current_exception;
}

namespace roo_testing::interrupt_delivery {

bool CanRegister() { return current_exception == 0; }

bool InstallDispatcher(void (*dispatch)()) {
  if (dispatch_pending != nullptr) return dispatch_pending == dispatch;
  dispatch_pending = dispatch;
  return true;
}

uint32_t Mask() {
  const bool was_masked = interrupts_masked;
  interrupts_masked = true;
  return was_masked ? 1 : 0;
}

void Restore(uint32_t state) { interrupts_masked = state != 0; }

void RequestDispatch() noexcept {
  dispatch_requested.store(true, std::memory_order_release);
}

bool CanDispatch() {
  if (!interrupts_masked) return true;
  // A timer handler can mask interrupts after dispatch has already started.
  // Leave the remaining work requested until the core restores its mask.
  RequestDispatch();
  return false;
}

void DispatchPicoInterrupts() {
  if (current_exception != 0) return;
  while (!interrupts_masked &&
         dispatch_requested.exchange(false, std::memory_order_acquire)) {
    dispatch_pending();
  }
}

void EnterPicoTimerInterrupt(unsigned alarm_num) {
  current_exception = kFirstExternalException + alarm_num;
}

void LeavePicoTimerInterrupt() { current_exception = 0; }

}  // namespace roo_testing::interrupt_delivery
