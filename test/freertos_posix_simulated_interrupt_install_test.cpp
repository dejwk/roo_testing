#include <chrono>
#include <csignal>

#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing_port.h"

namespace {

volatile sig_atomic_t handler_count = 0;
volatile sig_atomic_t handler_observed_isr = 0;

void SimulatedInterruptDispatcher() {
  handler_count = handler_count + 1;
  handler_observed_isr = xPortInIsrContext() == pdTRUE ? 1 : -1;
}

void AlternateDispatcher() {}

bool SpinUntil(volatile sig_atomic_t* value, sig_atomic_t expected) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (*value != expected && std::chrono::steady_clock::now() < deadline) {
  }
  return *value == expected;
}

}  // namespace

// Verifies requests survive the period before one-time dispatcher installation
// and that the installed dispatcher's lifetime cannot subsequently be changed.
TEST(FreeRtosPosixSimulatedInterruptInstall, PreservesPendingRequest) {
  EXPECT_FALSE(xPortInstallSimulatedInterruptDispatcher(nullptr));
  vPortRequestSimulatedInterrupt();
  ASSERT_TRUE(
      xPortInstallSimulatedInterruptDispatcher(SimulatedInterruptDispatcher));

  EXPECT_TRUE(SpinUntil(&handler_count, 1));
  EXPECT_EQ(handler_observed_isr, 1);
  EXPECT_TRUE(
      xPortInstallSimulatedInterruptDispatcher(SimulatedInterruptDispatcher));
  EXPECT_FALSE(xPortInstallSimulatedInterruptDispatcher(AlternateDispatcher));
}
