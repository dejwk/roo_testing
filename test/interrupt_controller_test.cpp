#include "roo_testing/interrupts/interrupt_controller.h"

#include <chrono>
#include <csignal>

#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"

namespace {

enum class HandlerAction : sig_atomic_t {
  kNone,
  kReassertOnce,
  kDisableAndReassert,
};

struct HandlerState {
  volatile sig_atomic_t count = 0;
  volatile sig_atomic_t observed_isr = 0;
  volatile sig_atomic_t action =
      static_cast<sig_atomic_t>(HandlerAction::kNone);
  roo_testing::InterruptHandle handle;
};

void InterruptHandler(void* argument) {
  HandlerState* state = static_cast<HandlerState*>(argument);
  state->count = state->count + 1;
  state->observed_isr = xPortInIsrContext() == pdTRUE ? 1 : -1;
  const HandlerAction action = static_cast<HandlerAction>(state->action);
  if (action == HandlerAction::kReassertOnce && state->count == 1) {
    roo_testing::setInterruptPending(state->handle);
  } else if (action == HandlerAction::kDisableAndReassert &&
             state->count == 1) {
    roo_testing::disableInterrupt(state->handle);
    roo_testing::setInterruptPending(state->handle);
  }
}

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

roo_testing::InterruptHandle Register(HandlerState* state,
                                      bool initially_enabled = true) {
  roo_testing::InterruptHandle handle;
  EXPECT_EQ(roo_testing::registerInterrupt(InterruptHandler, state,
                                           initially_enabled, &handle),
            roo_testing::InterruptRegistrationResult::kRegistered);
  state->handle = handle;
  return handle;
}

}  // namespace

// Verifies multiple assertions before delivery coalesce into one ISR call.
TEST(InterruptController, CoalescesPendingDelivery) {
  HandlerState state;
  const roo_testing::InterruptHandle handle = Register(&state);
  ASSERT_TRUE(handle.isValid());

  portENTER_CRITICAL(nullptr);
  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  portEXIT_CRITICAL(nullptr);

  EXPECT_TRUE(SpinUntil(&state.count, 1));
  EXPECT_EQ(state.observed_isr, 1);
  EXPECT_TRUE(roo_testing::unregisterInterrupt(handle));
}

// Verifies disabling retains pending work and enabling dispatches it.
TEST(InterruptController, RetainsPendingWhileDisabled) {
  HandlerState state;
  const roo_testing::InterruptHandle handle = Register(&state, false);
  ASSERT_TRUE(handle.isValid());

  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  EXPECT_EQ(state.count, 0);
  EXPECT_TRUE(roo_testing::enableInterrupt(handle));
  EXPECT_TRUE(SpinUntil(&state.count, 1));

  EXPECT_TRUE(roo_testing::disableInterrupt(handle));
  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  EXPECT_EQ(state.count, 1);
  EXPECT_TRUE(roo_testing::enableInterrupt(handle));
  EXPECT_TRUE(SpinUntil(&state.count, 2));
  EXPECT_TRUE(roo_testing::unregisterInterrupt(handle));
}

// Verifies a recycled slot rejects a handle from its previous generation.
TEST(InterruptController, RejectsStaleGeneration) {
  HandlerState old_state;
  const roo_testing::InterruptHandle old_handle = Register(&old_state);
  ASSERT_TRUE(roo_testing::unregisterInterrupt(old_handle));

  HandlerState new_state;
  const roo_testing::InterruptHandle new_handle = Register(&new_state);
  ASSERT_EQ(new_handle.slot, old_handle.slot);
  ASSERT_NE(new_handle.generation, old_handle.generation);

  EXPECT_FALSE(roo_testing::setInterruptPending(old_handle));
  EXPECT_TRUE(roo_testing::setInterruptPending(new_handle));
  EXPECT_TRUE(SpinUntil(&new_state.count, 1));
  EXPECT_EQ(old_state.count, 0);
  EXPECT_TRUE(roo_testing::unregisterInterrupt(new_handle));
}

// Verifies a handler can reassert itself without nesting dispatcher frames.
TEST(InterruptController, DrainsHandlerReassertion) {
  HandlerState state;
  state.action = static_cast<sig_atomic_t>(HandlerAction::kReassertOnce);
  const roo_testing::InterruptHandle handle = Register(&state);

  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  EXPECT_TRUE(SpinUntil(&state.count, 2));
  EXPECT_EQ(state.observed_isr, 1);
  EXPECT_TRUE(roo_testing::unregisterInterrupt(handle));
}

// Verifies enable and disable remain usable from the emulated ISR itself.
TEST(InterruptController, HandlerCanDisableAndLatchPending) {
  HandlerState state;
  state.action = static_cast<sig_atomic_t>(HandlerAction::kDisableAndReassert);
  const roo_testing::InterruptHandle handle = Register(&state);

  EXPECT_TRUE(roo_testing::setInterruptPending(handle));
  EXPECT_TRUE(SpinUntil(&state.count, 1));
  EXPECT_TRUE(roo_testing::enableInterrupt(handle));
  EXPECT_TRUE(SpinUntil(&state.count, 2));
  EXPECT_TRUE(roo_testing::unregisterInterrupt(handle));
}
