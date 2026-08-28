#include <array>
#include <chrono>
#include <csignal>
#include <cstdint>

#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"

namespace {

constexpr int kExhaustionSource = 34567;

struct HandlerState {
  volatile sig_atomic_t count = 0;
  volatile sig_atomic_t observed_isr = 0;
};

void InterruptHandler(void* argument) {
  HandlerState* state = static_cast<HandlerState*>(argument);
  state->count = state->count + 1;
  state->observed_isr = xPortInIsrContext() == pdTRUE ? 1 : -1;
}

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

}  // namespace

// The test-only adapter has a maximum epoch of two. Every pair of lifetimes
// consumes and retires one vector/registration slot. All capacity must
// eventually exhaust instead of wrapping either identity back to epoch one.
TEST(EspInterruptAllocatorEpochExhaustion, RetiresSlotsWithoutAliasing) {
  constexpr size_t kSlotCount = 64;
  constexpr size_t kEpochCount = 2;
  constexpr size_t kLifetimeCount = kSlotCount * kEpochCount;
  std::array<HandlerState, kLifetimeCount> states{};
  std::array<intr_handle_t, kLifetimeCount> handles{};
  for (size_t i = 0; i < states.size(); ++i) {
    intr_handle_t handle = nullptr;
    ASSERT_EQ(esp_intr_alloc(kExhaustionSource, 0, InterruptHandler, &states[i],
                             &handle),
              ESP_OK)
        << "lifetime " << i;
    handles[i] = handle;
    if (i % kEpochCount == 1) {
      EXPECT_EQ(handles[i], handles[i - 1]) << "lifetime " << i;
    } else if (i != 0) {
      EXPECT_NE(handles[i], handles[i - 1]) << "lifetime " << i;
    }

    roo_testing::esp_idf::raiseInterruptSource(kExhaustionSource);
    ASSERT_TRUE(SpinUntil(&states[i].count, 1)) << "lifetime " << i;
    EXPECT_EQ(states[i].observed_isr, 1);
    ASSERT_EQ(esp_intr_free(handle), ESP_OK);
  }

  intr_handle_t sentinel = reinterpret_cast<intr_handle_t>(uintptr_t{1});
  intr_handle_t output = sentinel;
  EXPECT_EQ(
      esp_intr_alloc(kExhaustionSource, 0, InterruptHandler, nullptr, &output),
      ESP_ERR_NOT_FOUND);
  EXPECT_EQ(output, sentinel);

  roo_testing::esp_idf::raiseInterruptSource(kExhaustionSource);
  for (size_t i = 0; i < states.size(); ++i) {
    EXPECT_EQ(states[i].count, 1) << "retired lifetime " << i;
    EXPECT_EQ(states[i].observed_isr, 1) << "lifetime " << i;
  }
}
