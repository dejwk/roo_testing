#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"
#include "roo_testing/interrupts/interrupt_controller.h"

namespace {

void InterruptHandler(void*) {}

}  // namespace

// Verifies every slot is retired after its final generation and no stale
// handle becomes valid again when registration continues to later slots.
TEST(InterruptControllerExhaustion, RetiresSlotsWithoutGenerationWrap) {
  size_t retired_slots = 0;
  uint32_t previous_slot = static_cast<uint32_t>(-1);

  for (;;) {
    roo_testing::InterruptHandle first_handle;
    const roo_testing::InterruptRegistrationResult first_result =
        roo_testing::registerInterrupt(InterruptHandler, nullptr, false,
                                       &first_handle);
    if (first_result == roo_testing::InterruptRegistrationResult::kNoCapacity) {
      break;
    }
    ASSERT_EQ(first_result,
              roo_testing::InterruptRegistrationResult::kRegistered);
    ASSERT_EQ(first_handle.generation, 1u);
    if (retired_slots != 0) {
      ASSERT_GT(first_handle.slot, previous_slot);
    }
    ASSERT_TRUE(roo_testing::unregisterInterrupt(first_handle));

    roo_testing::InterruptHandle final_handle;
    ASSERT_EQ(roo_testing::registerInterrupt(InterruptHandler, nullptr, false,
                                             &final_handle),
              roo_testing::InterruptRegistrationResult::kRegistered);
    ASSERT_EQ(final_handle.slot, first_handle.slot);
    ASSERT_EQ(final_handle.generation, 2u);
    EXPECT_FALSE(roo_testing::setInterruptPending(first_handle));
    ASSERT_TRUE(roo_testing::unregisterInterrupt(final_handle));

    EXPECT_FALSE(roo_testing::setInterruptPending(first_handle));
    EXPECT_FALSE(roo_testing::setInterruptPending(final_handle));
    EXPECT_FALSE(roo_testing::enableInterrupt(final_handle));
    EXPECT_FALSE(roo_testing::disableInterrupt(final_handle));
    EXPECT_FALSE(roo_testing::unregisterInterrupt(final_handle));

    previous_slot = final_handle.slot;
    ++retired_slots;
  }

  EXPECT_GT(retired_slots, 0u);
  roo_testing::InterruptHandle exhausted_handle;
  EXPECT_EQ(roo_testing::registerInterrupt(InterruptHandler, nullptr, false,
                                           &exhausted_handle),
            roo_testing::InterruptRegistrationResult::kNoCapacity);
}
