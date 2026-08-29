#include "roo_testing/frameworks/esp32_shims/ledc_fade_engine.h"

#include <gtest/gtest.h>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

namespace {

LedcFadeEngine::Request Request(uint8_t channel, int pin, uint64_t duration) {
  return {channel, pin, 1000, 4, 0, 8, 0, 0.0, false, duration};
}

TEST(LedcFadeEngineTest, PublishesEnvelopeAndExactEndpoint) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(0, 30, 100)));
  auto signal = FakeEsp32().gpio.get(30).lastSignal();
  ASSERT_TRUE(signal.has_value());
  const auto& square =
      std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
  EXPECT_TRUE(std::holds_alternative<roo_testing_transducers::LinearDutyFade>(
      square.duty));
  system_time_lag_ns(50000);
  ProcessSystemTimeAlarms();
  EXPECT_EQ(4U, engine.snapshot(0).duty);
  system_time_lag_ns(50000);
  ProcessSystemTimeAlarms();
  EXPECT_EQ(8U, engine.snapshot(0).duty);
  EXPECT_TRUE(engine.snapshot(0).completion_pending);
  const auto completion = engine.TakeCompletion(0);
  ASSERT_TRUE(completion.has_value());
  EXPECT_EQ(8U, completion->duty);
  EXPECT_FALSE(engine.snapshot(0).gate_held);
}

TEST(LedcFadeEngineTest, ZeroAndEqualFadesCompleteThroughMailbox) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(1, 31, 0)));
  EXPECT_TRUE(engine.snapshot(1).completion_pending);
  EXPECT_TRUE(engine.TakeCompletion(1).has_value());
  auto equal = Request(2, 32, 100);
  equal.start_duty = 5;
  equal.target_duty = 5;
  ASSERT_TRUE(engine.Start(equal));
  EXPECT_TRUE(engine.TakeCompletion(2).has_value());
}

TEST(LedcFadeEngineTest, CancellationMaterializesAndSuppressesStaleAlarm) {
  LedcFadeEngine engine;
  ASSERT_TRUE(engine.Start(Request(3, 33, 100)));
  system_time_lag_ns(50000);
  ASSERT_TRUE(engine.Cancel(3));
  EXPECT_EQ(4U, engine.snapshot(3).duty);
  system_time_lag_ns(100000);
  ProcessSystemTimeAlarms();
  EXPECT_FALSE(engine.snapshot(3).completion_pending);
}

TEST(LedcFadeEngineTest, EndpointPublicationPrecedesCompletionMailbox) {
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
  FakeEsp32().gpio.attachOutput(34, sink);
  auto request = Request(4, 34, 10);
  request.target_duty = 8;
  ASSERT_TRUE(engine.Start(request));
  system_time_lag_ns(10000);
  ProcessSystemTimeAlarms();
  EXPECT_FALSE(endpoint_saw_mailbox);
  EXPECT_TRUE(engine.snapshot(4).completion_pending);
}

}  // namespace
