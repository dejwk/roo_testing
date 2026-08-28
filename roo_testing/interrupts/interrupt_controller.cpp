#include "roo_testing/interrupts/interrupt_controller.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "roo_testing_port.h"

namespace roo_testing {
namespace {

constexpr size_t kInterruptCapacity = 64;
constexpr uint64_t kAllocated = uint64_t{1} << 0;
constexpr uint64_t kEnabled = uint64_t{1} << 1;
constexpr uint64_t kPending = uint64_t{1} << 2;
constexpr unsigned kGenerationShift = 32;
// A test-only target shortens the identity space to exercise retirement.
#ifdef ROO_TESTING_INTERNAL_INTERRUPT_MAX_GENERATION
constexpr uint32_t kMaxGeneration =
    ROO_TESTING_INTERNAL_INTERRUPT_MAX_GENERATION;
#else
constexpr uint32_t kMaxGeneration = std::numeric_limits<uint32_t>::max();
#endif

static_assert(kMaxGeneration > 0,
              "interrupt registrations need a nonzero generation");

struct InterruptSlot {
  std::atomic<uint64_t> state{0};
  std::atomic<InterruptHandler> handler{nullptr};
  std::atomic<void*> argument{nullptr};
};

std::array<InterruptSlot, kInterruptCapacity> interrupt_slots;

static_assert(std::atomic<uint64_t>::is_always_lock_free,
              "interrupt state must be lock-free in signal context");
static_assert(std::atomic<InterruptHandler>::is_always_lock_free,
              "interrupt handlers must be lock-free in signal context");
static_assert(std::atomic<void*>::is_always_lock_free,
              "interrupt arguments must be lock-free in signal context");

uint32_t Generation(uint64_t state) {
  return static_cast<uint32_t>(state >> kGenerationShift);
}

uint64_t StateWithGeneration(uint32_t generation, uint64_t flags) {
  return (static_cast<uint64_t>(generation) << kGenerationShift) | flags;
}

// Returns whether allocating the slot can produce a never-before-used handle.
bool HasUnusedGeneration(uint64_t state) {
  return Generation(state) < kMaxGeneration;
}

bool IsTaskOperationContext() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
         xPortIsFreeRtosTask() == pdTRUE && xPortInIsrContext() == pdFALSE;
}

bool Matches(uint64_t state, InterruptHandle handle) {
  return (state & kAllocated) != 0 && Generation(state) == handle.generation;
}

// Claims pending handlers before invocation so a handler can reassert itself.
void DispatchPendingInterrupts() {
  for (;;) {
    bool dispatched = false;
    for (InterruptSlot& slot : interrupt_slots) {
      uint64_t state = slot.state.load(std::memory_order_acquire);
      while ((state & (kAllocated | kEnabled | kPending)) ==
             (kAllocated | kEnabled | kPending)) {
        if (!slot.state.compare_exchange_weak(state, state & ~kPending,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire)) {
          continue;
        }
        InterruptHandler handler = slot.handler.load(std::memory_order_acquire);
        void* argument = slot.argument.load(std::memory_order_acquire);
        if (handler != nullptr) handler(argument);
        dispatched = true;
        break;
      }
    }
    if (!dispatched) return;
  }
}

}  // namespace

InterruptRegistrationResult registerInterrupt(InterruptHandler handler,
                                              void* argument,
                                              bool initially_enabled,
                                              InterruptHandle* out_handle) {
  if (handler == nullptr || out_handle == nullptr) {
    return InterruptRegistrationResult::kInvalidArgument;
  }
  if (!IsTaskOperationContext()) {
    return InterruptRegistrationResult::kWrongContext;
  }

  portENTER_CRITICAL(nullptr);
  if (!xPortInstallSimulatedInterruptDispatcher(DispatchPendingInterrupts)) {
    portEXIT_CRITICAL(nullptr);
    return InterruptRegistrationResult::kBackendUnavailable;
  }

  for (size_t i = 0; i < interrupt_slots.size(); ++i) {
    InterruptSlot& slot = interrupt_slots[i];
    const uint64_t old_state = slot.state.load(std::memory_order_relaxed);
    if ((old_state & kAllocated) != 0 || !HasUnusedGeneration(old_state)) {
      continue;
    }

    const uint32_t generation = Generation(old_state) + 1;
    slot.handler.store(handler, std::memory_order_relaxed);
    slot.argument.store(argument, std::memory_order_relaxed);
    const uint64_t flags = kAllocated | (initially_enabled ? kEnabled : 0);
    slot.state.store(StateWithGeneration(generation, flags),
                     std::memory_order_release);
    *out_handle = {static_cast<uint32_t>(i), generation};
    portEXIT_CRITICAL(nullptr);
    return InterruptRegistrationResult::kRegistered;
  }

  portEXIT_CRITICAL(nullptr);
  return InterruptRegistrationResult::kNoCapacity;
}

bool unregisterInterrupt(InterruptHandle handle) {
  if (!IsTaskOperationContext() || !handle.isValid() ||
      handle.slot >= interrupt_slots.size()) {
    return false;
  }

  InterruptSlot& slot = interrupt_slots[handle.slot];
  portENTER_CRITICAL(nullptr);
  uint64_t state = slot.state.load(std::memory_order_acquire);
  while (Matches(state, handle)) {
    if (slot.state.compare_exchange_weak(
            state, StateWithGeneration(handle.generation, 0),
            std::memory_order_acq_rel, std::memory_order_acquire)) {
      slot.handler.store(nullptr, std::memory_order_relaxed);
      slot.argument.store(nullptr, std::memory_order_relaxed);
      portEXIT_CRITICAL(nullptr);
      return true;
    }
  }
  portEXIT_CRITICAL(nullptr);
  return false;
}

bool enableInterrupt(InterruptHandle handle) noexcept {
  if (!handle.isValid() || handle.slot >= interrupt_slots.size()) {
    return false;
  }

  InterruptSlot& slot = interrupt_slots[handle.slot];
  uint64_t state = slot.state.load(std::memory_order_acquire);
  while (Matches(state, handle)) {
    const uint64_t enabled_state = state | kEnabled;
    if (slot.state.compare_exchange_weak(state, enabled_state,
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire)) {
      if ((enabled_state & kPending) != 0) vPortRequestSimulatedInterrupt();
      return true;
    }
  }
  return false;
}

bool disableInterrupt(InterruptHandle handle) noexcept {
  if (!handle.isValid() || handle.slot >= interrupt_slots.size()) {
    return false;
  }

  InterruptSlot& slot = interrupt_slots[handle.slot];
  uint64_t state = slot.state.load(std::memory_order_acquire);
  while (Matches(state, handle)) {
    if (slot.state.compare_exchange_weak(state, state & ~kEnabled,
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire)) {
      return true;
    }
  }
  return false;
}

bool setInterruptPending(InterruptHandle handle) noexcept {
  if (!handle.isValid() || handle.slot >= interrupt_slots.size()) return false;

  InterruptSlot& slot = interrupt_slots[handle.slot];
  uint64_t state = slot.state.load(std::memory_order_acquire);
  while (Matches(state, handle)) {
    const uint64_t pending_state = state | kPending;
    if (slot.state.compare_exchange_weak(state, pending_state,
                                         std::memory_order_acq_rel,
                                         std::memory_order_acquire)) {
      if ((pending_state & kEnabled) != 0) {
        vPortRequestSimulatedInterrupt();
      }
      return true;
    }
  }
  return false;
}

}  // namespace roo_testing
