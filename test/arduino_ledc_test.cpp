#include <gtest/gtest.h>

#include "esp32-hal-ledc.h"
#include "esp32-hal.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

namespace {

bool callback_invoked = false;

void FadeCallback() { callback_invoked = true; }

roo_testing_transducers::SquareVoltageSpec SquareSignal(uint8_t pin) {
  const auto signal = FakeEsp32().gpio.get(pin).lastSignal();
  if (!signal.has_value()) {
    ADD_FAILURE() << "pin " << static_cast<int>(pin) << " has no signal";
    return {{0.0f, 0.0f, 0.0, 0},
            roo_testing_transducers::ConstantDuty{0.0},
            0.0,
            false};
  }
  return std::get<roo_testing_transducers::SquareVoltageSpec>(signal->spec());
}

TEST(ArduinoLedcTest, UnsupportedFadesDoNotChangeOutputOrInvokeCallbacks) {
  ASSERT_TRUE(ledcAttachChannel(23, 1000, 8, 0));
  ASSERT_TRUE(ledcWrite(23, 37));

  EXPECT_FALSE(ledcFade(23, 1, 100, 10));
  EXPECT_FALSE(ledcFadeWithInterrupt(23, 1, 100, 10, FadeCallback));
  EXPECT_FALSE(ledcFadeWithInterruptArg(
      23, 1, 100, 10, [](void* value) { *static_cast<bool*>(value) = true; },
      &callback_invoked));
  EXPECT_EQ(37U, ledcRead(23));
  EXPECT_FALSE(callback_invoked);
}

TEST(ArduinoLedcTest, PublishesMappedDutyAndAnalogWriteOutput) {
  ASSERT_TRUE(ledcAttachChannel(24, 1000, 8, 1));
  ASSERT_TRUE(ledcWrite(24, 255));
  EXPECT_EQ(256U, ledcRead(24));
  const auto full_on = SquareSignal(24);
  EXPECT_DOUBLE_EQ(
      1.0, std::get<roo_testing_transducers::ConstantDuty>(full_on.duty).duty);

  analogWrite(29, 127);
  EXPECT_EQ(127U, ledcRead(29));
  const auto analog = SquareSignal(29);
  EXPECT_DOUBLE_EQ(
      127.0 / 256.0,
      std::get<roo_testing_transducers::ConstantDuty>(analog.duty).duty);
}

TEST(ArduinoLedcTest, PublishesToneFrequencyAndInvertedIdleRail) {
  ASSERT_TRUE(ledcAttachChannel(25, 1000, 8, 2));
  EXPECT_EQ(440U, ledcWriteTone(25, 440));
  EXPECT_EQ(128U, ledcRead(25));
  const auto tone = SquareSignal(25);
  EXPECT_DOUBLE_EQ(440.0, tone.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      0.5, std::get<roo_testing_transducers::ConstantDuty>(tone.duty).duty);
  EXPECT_EQ(440U, ledcWriteNote(25, NOTE_A, 4));

  ASSERT_TRUE(ledcOutputInvert(25, true));
  EXPECT_EQ(0U, ledcWriteTone(25, 0));
  const auto idle = FakeEsp32().gpio.get(25).lastSignal();
  ASSERT_TRUE(idle.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            idle->kind());
  EXPECT_FLOAT_EQ(roo_testing_transducers::VoltageDigitalHigh(),
                  idle->voltageAtUptimeMicros(0));
}

TEST(ArduinoLedcTest, RepublishesChangesAndDrivesDetachedAndReusedPinsLow) {
  ASSERT_TRUE(ledcAttachChannel(26, 1000, 8, 3));
  ASSERT_TRUE(ledcWriteChannel(3, 64));
  EXPECT_EQ(2000U, ledcChangeFrequency(26, 2000, 4));
  EXPECT_EQ(16U, ledcRead(26));
  const auto changed = SquareSignal(26);
  EXPECT_DOUBLE_EQ(2000.0, changed.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      1.0, std::get<roo_testing_transducers::ConstantDuty>(changed.duty).duty);
  ASSERT_TRUE(ledcOutputInvert(26, true));
  EXPECT_TRUE(SquareSignal(26).inverted);

  ASSERT_TRUE(ledcAttachChannel(27, 1000, 8, 4));
  ASSERT_TRUE(ledcWrite(27, 20));
  ASSERT_TRUE(ledcAttachChannel(28, 1000, 8, 4));
  EXPECT_EQ(0U, ledcRead(28));
  const auto old_pin = FakeEsp32().gpio.get(27).lastSignal();
  ASSERT_TRUE(old_pin.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            old_pin->kind());
  EXPECT_FLOAT_EQ(0.0f, old_pin->voltageAtUptimeMicros(0));
  ASSERT_TRUE(ledcDetach(28));
  const auto detached = FakeEsp32().gpio.get(28).lastSignal();
  ASSERT_TRUE(detached.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            detached->kind());
  EXPECT_FLOAT_EQ(0.0f, detached->voltageAtUptimeMicros(0));
}

}  // namespace
