#include "roo_testing/transducers/voltage/voltage_signal.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

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

}  // namespace
}  // namespace roo_testing_transducers
