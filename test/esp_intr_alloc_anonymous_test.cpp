#include <chrono>
#include <csignal>

#include "esp_intr_alloc.h"
#include "gtest/gtest.h"
#include "roo_testing/frameworks/esp_idf_support/esp_interrupts.h"

namespace {

constexpr int kTestSource = 23456;
volatile sig_atomic_t handler_count = 0;

void InterruptHandler(void*) { handler_count = handler_count + 1; }

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

}  // namespace

// Verifies a null output handle still leaves a live interrupt allocation.
TEST(EspInterruptAllocatorAnonymous, KeepsRegistration) {
  ASSERT_EQ(esp_intr_alloc(kTestSource, 0, InterruptHandler, nullptr, nullptr),
            ESP_OK);

  roo_testing::esp_idf::raiseInterruptSource(kTestSource);
  EXPECT_TRUE(SpinUntil(&handler_count, 1));
}
