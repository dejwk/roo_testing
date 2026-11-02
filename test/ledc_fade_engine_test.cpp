#include "roo_testing/frameworks/esp32_shims/ledc_fade_engine.h"

#include <gtest/gtest.h>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

namespace {

// Auto-sync may advance while the host schedules this test. Use a long fade
// and explicit virtual advances rather than a sub-millisecond timing window.
constexpr uint64_t kFadeMicros = 10000000;

class LedcFadeEngineTest : public testing::Test {
 protected:
  void SetUp() override {
    EXPECT_EQ(ROO_TESTING_EXPECT_AUTO_SYNC != 0,
              system_time_is_auto_sync_enabled());
  }
};

LedcFadeEngine::Request Request(uint8_t channel, int pin, uint64_t duration) {
  return {channel, pin, 1000, 4, 0, 8, 0, 0.0, false, duration};
}

// Verifies interpolation publishes a fade, then the exact endpoint/mailbox.
TEST_F(LedcFadeEngineTest, PublishesEnvelopeAndExactEndpoint) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(0, 2, kFadeMicros)));
  auto signal = FakeEsp32().gpio.get(2).lastSignal();
  ASSERT_TRUE(signal.has_value());
  const auto& square =
      std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
  EXPECT_TRUE(std::holds_alternative<roo_testing_transducers::LinearDutyFade>(
      square.duty));
  system_time_lag_ns(kFadeMicros * 500);
  ProcessSystemTimeAlarms();
  const uint32_t midpoint = engine.snapshot(0).duty;
  if (system_time_is_auto_sync_enabled()) {
    EXPECT_GE(midpoint, 4U);
    EXPECT_LT(midpoint, 8U);
  } else {
    EXPECT_EQ(4U, midpoint);
  }
  system_time_lag_ns(kFadeMicros * 500);
  ProcessSystemTimeAlarms();
  EXPECT_EQ(8U, engine.snapshot(0).duty);
  EXPECT_TRUE(engine.snapshot(0).completion_pending);
  const auto completion = engine.TakeCompletion(0);
  ASSERT_TRUE(completion.has_value());
  EXPECT_EQ(8U, completion->duty);
  EXPECT_FALSE(engine.snapshot(0).gate_held);
}

// Verifies immediate and equal-duty fades still report completion.
TEST_F(LedcFadeEngineTest, ZeroAndEqualFadesCompleteThroughMailbox) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(1, 3, 0)));
  EXPECT_TRUE(engine.snapshot(1).completion_pending);
  EXPECT_TRUE(engine.TakeCompletion(1).has_value());
  auto equal = Request(2, 4, 100);
  equal.start_duty = 5;
  equal.target_duty = 5;
  ASSERT_TRUE(engine.Start(equal));
  EXPECT_TRUE(engine.TakeCompletion(2).has_value());
}

// Verifies cancellation preserves current duty and suppresses stale completion.
TEST_F(LedcFadeEngineTest, CancellationMaterializesAndSuppressesStaleAlarm) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(3, 5, kFadeMicros)));
  system_time_lag_ns(kFadeMicros * 500);
  ASSERT_TRUE(engine.Cancel(3));
  const uint32_t midpoint = engine.snapshot(3).duty;
  if (system_time_is_auto_sync_enabled()) {
    EXPECT_GE(midpoint, 4U);
    EXPECT_LT(midpoint, 8U);
  } else {
    EXPECT_EQ(4U, midpoint);
  }
  system_time_lag_ns(kFadeMicros * 1000);
  ProcessSystemTimeAlarms();
  EXPECT_FALSE(engine.snapshot(3).completion_pending);
}

// Verifies sinks see the endpoint before the completion mailbox is published.
TEST_F(LedcFadeEngineTest, EndpointPublicationPrecedesCompletionMailbox) {
  LedcFadeEngine engine;
  bool endpoint_saw_mailbox = true;
  roo_testing_transducers::SimpleVoltageSink sink =
      roo_testing_transducers::SimpleVoltageSink::WithSignalCallback(
          "ordering",
          [&](const roo_testing_transducers::VoltageSignal& signal) {
            if (signal.kind() ==
                    roo_testing_transducers::VoltageSignalKind::kSquare &&
                signal.activeDutyAtUptimeMicros(system_time_get_micros()) ==
                    0.5) {
              endpoint_saw_mailbox = engine.snapshot(4).completion_pending;
            }
          });
  FakeEsp32().gpio.attachOutput(6, sink);
  auto request = Request(4, 6, 10);
  request.target_duty = 8;
  ASSERT_TRUE(engine.Start(request));
  system_time_lag_ns(10000);
  ProcessSystemTimeAlarms();
  EXPECT_FALSE(endpoint_saw_mailbox);
  EXPECT_TRUE(engine.snapshot(4).completion_pending);
}

// Verifies a sink cannot mutate the engine reentrantly during delivery.
TEST_F(LedcFadeEngineTest, RejectsMutationReenteredFromSinkDelivery) {
  LedcFadeEngine engine;
  bool rejected = false;
  roo_testing_transducers::SimpleVoltageSink sink =
      roo_testing_transducers::SimpleVoltageSink::WithSignalCallback(
          "reentry", [&](const roo_testing_transducers::VoltageSignal&) {
            rejected = !engine.Cancel(5);
          });
  FakeEsp32().gpio.attachOutput(7, sink);
  ASSERT_TRUE(engine.Start(Request(5, 7, kFadeMicros)));
  EXPECT_TRUE(rejected);
  EXPECT_TRUE(engine.snapshot(5).gate_held);
  ASSERT_TRUE(engine.Cancel(5));
}

}  // namespace
