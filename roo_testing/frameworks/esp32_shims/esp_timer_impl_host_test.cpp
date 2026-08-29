#include <atomic>
#include <cstdint>
#include <limits>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "gtest/gtest.h"
#include "roo_testing/system/timer.h"

extern "C" {
#include "components/esp_timer/private_include/esp_timer_impl.h"
}

namespace {

std::atomic<int> handler_calls{0};
std::atomic<bool> handler_in_isr{false};

void AlarmHandler(void*) {
  handler_in_isr.store(xPortInIsrContext(), std::memory_order_relaxed);
  handler_calls.fetch_add(1, std::memory_order_relaxed);
}

class EspTimerImplHostTest : public testing::Test {
 protected:
  void SetUp() override {
    handler_calls.store(0, std::memory_order_relaxed);
    handler_in_isr.store(false, std::memory_order_relaxed);
    ASSERT_EQ(ESP_OK, esp_timer_impl_early_init());
    ASSERT_EQ(ESP_OK, esp_timer_impl_init(AlarmHandler));
  }

  void TearDown() override { esp_timer_impl_deinit(); }
};

// Verifies the host counter is the shared emulated uptime before and after
// init.
TEST(EspTimerImplHostEarlyTest, ReadsSharedUptime) {
  ASSERT_EQ(ESP_OK, esp_timer_impl_early_init());
  EXPECT_EQ(system_time_get_micros(), esp_timer_impl_get_time());
  EXPECT_EQ(static_cast<uint64_t>(system_time_get_micros()),
            esp_timer_impl_get_counter_reg());
}

// Verifies a due compare is delivered through the emulated interrupt handler.
TEST_F(EspTimerImplHostTest, ArmsAndDeliversOnlyFromIsr) {
  const uint64_t deadline =
      static_cast<uint64_t>(system_time_get_micros() + 10);
  esp_timer_impl_set_alarm(deadline);
  EXPECT_EQ(deadline, esp_timer_impl_get_alarm_reg());

  system_time_delay_micros(10);

  EXPECT_EQ(1, handler_calls.load(std::memory_order_relaxed));
  EXPECT_TRUE(handler_in_isr.load(std::memory_order_relaxed));
}

// Verifies initialization owns exactly one interrupt registration at a time.
TEST_F(EspTimerImplHostTest, RejectsRepeatedInitialization) {
  EXPECT_EQ(ESP_ERR_INVALID_STATE, esp_timer_impl_init(AlarmHandler));
}

// Verifies rearming and disarming invalidate a previous compare registration.
TEST_F(EspTimerImplHostTest, RearmAndDisarmDiscardOldCompare) {
  const uint64_t now = static_cast<uint64_t>(system_time_get_micros());
  esp_timer_impl_set_alarm(now + 10);
  esp_timer_impl_set_alarm(now + 20);
  system_time_delay_micros(10);
  EXPECT_EQ(0, handler_calls.load(std::memory_order_relaxed));

  esp_timer_impl_set_alarm(std::numeric_limits<uint64_t>::max());
  system_time_delay_micros(20);
  EXPECT_EQ(0, handler_calls.load(std::memory_order_relaxed));
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(),
            esp_timer_impl_get_alarm_reg());
}

// Verifies an unrepresentable host deadline stays readable but is not armed.
TEST_F(EspTimerImplHostTest, LeavesExtremeTimestampDisarmed) {
  const uint64_t extreme = std::numeric_limits<uint64_t>::max() - 1;
  esp_timer_impl_set_alarm(extreme);
  EXPECT_EQ(extreme, esp_timer_impl_get_alarm_reg());
  ProcessSystemTimeAlarms();
  EXPECT_EQ(0, handler_calls.load(std::memory_order_relaxed));
}

// Verifies already-due compares wait for the normal explicit alarm pump.
TEST_F(EspTimerImplHostTest, DeliversAlreadyDueCompareWhenPumped) {
  const uint64_t now = static_cast<uint64_t>(system_time_get_micros());
  esp_timer_impl_set_alarm(now - 1);
  EXPECT_EQ(0, handler_calls.load(std::memory_order_relaxed));

  ProcessSystemTimeAlarms();
  EXPECT_EQ(1, handler_calls.load(std::memory_order_relaxed));
}

// Verifies deinit makes a pending compare and its interrupt registration inert.
TEST_F(EspTimerImplHostTest, DeinitCancelsPendingCompare) {
  const uint64_t deadline =
      static_cast<uint64_t>(system_time_get_micros() + 10);
  esp_timer_impl_set_alarm(deadline);
  esp_timer_impl_deinit();
  system_time_delay_micros(10);
  EXPECT_EQ(0, handler_calls.load(std::memory_order_relaxed));
}

}  // namespace
