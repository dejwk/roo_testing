#include "roo_testing/transducers/voltage/voltage_signal.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include "roo_testing/transducers/voltage/voltage.h"

namespace roo_testing_transducers {
namespace {

// Verifies each carrier shape has its documented phase-zero convention.
TEST(VoltageSignalTest, SamplesPeriodicCarriers) {
  EXPECT_NEAR(1.0, VoltageSignal::Sine(0, 2, 1, 0).voltageAtUptimeMicros(0),
              1e-6);
  EXPECT_NEAR(
      2.0, VoltageSignal::Sine(0, 2, 1, 0).voltageAtUptimeMicros(250000), 1e-6);
  EXPECT_NEAR(0.0, VoltageSignal::Triangle(0, 2, 1, 0).voltageAtUptimeMicros(0),
              1e-6);
  EXPECT_NEAR(2.0,
              VoltageSignal::Triangle(0, 2, 1, 0).voltageAtUptimeMicros(500000),
              1e-6);
  EXPECT_NEAR(1.0,
              VoltageSignal::Sawtooth(0, 2, 1, 0).voltageAtUptimeMicros(500000),
              1e-6);
  EXPECT_NEAR(
      1.0,
      VoltageSignal::Sawtooth(0, 2, 1, 0, 0, SawtoothDirection::kFalling)
          .voltageAtUptimeMicros(500000),
      1e-6);
}

// Verifies pulse placement, inversion, and half-open PWM boundaries.
TEST(VoltageSignalTest, SamplesSquareAndDutyEnvelope) {
  VoltageSignal signal =
      VoltageSignal::Square(0, 3.3f, 10, ConstantDuty{0.25}, 0, 0.5, true);
  EXPECT_FLOAT_EQ(3.3f, signal.voltageAtUptimeMicros(0));
  EXPECT_FLOAT_EQ(0, signal.voltageAtUptimeMicros(50000));
  EXPECT_FLOAT_EQ(3.3f, signal.voltageAtUptimeMicros(75000));
  VoltageSignal fade =
      VoltageSignal::Square(0, 1, 1, LinearDutyFade{0, 1, 100, 1000}, 0);
  EXPECT_DOUBLE_EQ(0, fade.activeDutyAtUptimeMicros(99));
  EXPECT_DOUBLE_EQ(0.5, fade.activeDutyAtUptimeMicros(600));
  EXPECT_DOUBLE_EQ(1, fade.activeDutyAtUptimeMicros(1100));
}

// Verifies exact local analytical properties, including PWM DC and RMS values.
TEST(VoltageSignalTest, AnalyzesSignals) {
  VoltageAnalysis pwm = AnalyzeVoltage(
      VoltageSignal::Square(0, 3.3f, 1000, ConstantDuty{0.5}, 0), 0);
  EXPECT_NEAR(1.65, pwm.dc_voltage, 1e-6);
  EXPECT_NEAR(2.333452, pwm.rms_voltage, 1e-5);
  EXPECT_NEAR(1.65, pwm.ac_rms_voltage, 1e-6);
  EXPECT_NEAR(1.65, pwm.ac_peak_voltage, 1e-6);
  VoltageAnalysis constant = AnalyzeVoltage(VoltageSignal::Constant(-2), 0);
  EXPECT_FLOAT_EQ(2, constant.rms_voltage);
  EXPECT_FLOAT_EQ(0, constant.ac_rms_voltage);
}

// Verifies normalized specification equality and robust extreme-time sampling.
TEST(VoltageSignalTest, NormalizesSpecificationsAndKeepsExtremePhaseFinite) {
  VoltageSignal first = VoltageSignal::Sine(0, 1, 1234567.25, INT64_MIN, -0.25);
  VoltageSignal second = VoltageSignal::Sine(0, 1, 1234567.25, INT64_MIN, 0.75);
  EXPECT_EQ(first, second);
  EXPECT_TRUE(std::isfinite(first.voltageAtUptimeMicros(INT64_MAX)));
}

// Verifies invalid signal configuration fails at the factory boundary.
TEST(VoltageSignalTest, RejectsInvalidConfiguration) {
  EXPECT_DEATH(VoltageSignal::Sine(1, 0, 1, 0), "");
  EXPECT_DEATH(VoltageSignal::Square(0, 1, 1, ConstantDuty{1.1}, 0), "");
  EXPECT_DEATH(VoltageSignal::Constant(std::numeric_limits<float>::infinity()),
               "");
}

// Verifies sinks retain complete signals while legacy views use local DC.
TEST(VoltageSignalTest, SimpleSinksPreserveSignalsAndUseExplicitViews) {
  const VoltageSignal pwm =
      VoltageSignal::Square(0, 3.3f, 1000, ConstantDuty{0.5}, 0);
  SimpleVoltageSink voltage_sink;
  SimpleDigitalSink digital_sink;
  voltage_sink.write(pwm);
  digital_sink.write(pwm);
  ASSERT_TRUE(voltage_sink.signal().has_value());
  EXPECT_EQ(pwm, *voltage_sink.signal());
  EXPECT_NEAR(1.65, voltage_sink.averageDcVoltageAtUptimeMicros(0), 1e-6);
  EXPECT_FLOAT_EQ(3.3f, voltage_sink.sampleAtUptimeMicros(0));
  EXPECT_EQ(kDigitalUndef, digital_sink.value());
  EXPECT_EQ(kDigitalHigh, digital_sink.instantaneousValueAtUptimeMicros(0));
}

// Verifies retained state is visible before callbacks and callbacks receive one
// assignment.
TEST(VoltageSignalTest, SimpleSinkCallbacksObserveCommittedSignal) {
  SimpleVoltageSink* sink_ptr = nullptr;
  SimpleVoltageSink sink = SimpleVoltageSink::WithSignalCallback(
      "sink", [&sink_ptr](const VoltageSignal& signal) {
        ASSERT_NE(nullptr, sink_ptr);
        ASSERT_TRUE(sink_ptr->signal().has_value());
        EXPECT_EQ(signal, *sink_ptr->signal());
      });
  sink_ptr = &sink;
  sink.write(VoltageSignal::Constant(1.2f));
}

}  // namespace
}  // namespace roo_testing_transducers
