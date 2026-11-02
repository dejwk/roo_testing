#include <gtest/gtest.h>

#include "esp32-hal-ledc.h"
#include "esp32-hal.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

namespace {

// Long fades leave room for host scheduling in auto-sync mode. Tests advance
// virtual time explicitly, so this does not add a corresponding real delay.
constexpr uint32_t kFadeMillis = 10000;
bool callback_invoked = false;

class ArduinoLedcTest : public testing::Test {
 protected:
  void SetUp() override {
    EXPECT_EQ(ROO_TESTING_EXPECT_AUTO_SYNC != 0,
              system_time_is_auto_sync_enabled());
    callback_invoked = false;
  }

  void TearDown() override {
    for (uint8_t pin = 2; pin <= 10; ++pin) {
      ledcDetach(pin);
    }
  }
};

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

// Verifies one active fade excludes a second fade on the same channel.
TEST_F(ArduinoLedcTest, StartsFadesAndRejectsConcurrentFade) {
  ASSERT_TRUE(ledcAttachChannel(2, 1000, 8, 0));
  ASSERT_TRUE(ledcWrite(2, 37));

  EXPECT_TRUE(ledcFade(2, 1, 100, kFadeMillis));
  EXPECT_FALSE(ledcFadeWithInterrupt(2, 1, 100, kFadeMillis, FadeCallback));
  EXPECT_GE(ledcRead(2), 1U);
  EXPECT_LE(ledcRead(2), 100U);
  EXPECT_FALSE(callback_invoked);
}

// Verifies a zero-duration fade publishes full duty and invokes its callback.
TEST_F(ArduinoLedcTest, ZeroDurationFadeCompletesAndInvokesCallbacks) {
  ASSERT_TRUE(ledcAttach(3, 1000, 8));
  callback_invoked = false;
  EXPECT_TRUE(ledcFadeWithInterrupt(3, 0, 255, 0, FadeCallback));
  EXPECT_EQ(256U, ledcRead(3));
  EXPECT_TRUE(callback_invoked);
}

// Verifies the published fade and readback advance to the exact endpoint.
TEST_F(ArduinoLedcTest, FadePublishesEnvelopeAndMaterializesReadback) {
  ASSERT_TRUE(ledcAttachChannel(4, 1000, 8, 5));
  ASSERT_TRUE(ledcFade(4, 0, 128, kFadeMillis));
  const auto signal = SquareSignal(4);
  EXPECT_TRUE(std::holds_alternative<roo_testing_transducers::LinearDutyFade>(
      signal.duty));
  system_time_lag_ns(static_cast<uint64_t>(kFadeMillis) * 500000);
  ProcessSystemTimeAlarms();
  const uint32_t midpoint = ledcRead(4);
  if (system_time_is_auto_sync_enabled()) {
    // Host time continues between the explicit advance and the read.
    EXPECT_GE(midpoint, 64U);
    EXPECT_LT(midpoint, 128U);
  } else {
    EXPECT_EQ(64U, midpoint);
  }
  system_time_lag_ns(static_cast<uint64_t>(kFadeMillis) * 500000);
  ProcessSystemTimeAlarms();
  EXPECT_EQ(128U, ledcRead(4));
}

// Verifies full-duty mapping and analogWrite publish the expected waveform.
TEST_F(ArduinoLedcTest, PublishesMappedDutyAndAnalogWriteOutput) {
  ASSERT_TRUE(ledcAttachChannel(5, 1000, 8, 1));
  ASSERT_TRUE(ledcWrite(5, 255));
  EXPECT_EQ(256U, ledcRead(5));
  const auto full_on = SquareSignal(5);
  EXPECT_DOUBLE_EQ(
      1.0, std::get<roo_testing_transducers::ConstantDuty>(full_on.duty).duty);

  analogWrite(6, 127);
  EXPECT_EQ(127U, ledcRead(6));
  const auto analog = SquareSignal(6);
  EXPECT_DOUBLE_EQ(
      127.0 / 256.0,
      std::get<roo_testing_transducers::ConstantDuty>(analog.duty).duty);
}

// Verifies tone/note frequency and the inverted idle voltage.
TEST_F(ArduinoLedcTest, PublishesToneFrequencyAndInvertedIdleRail) {
  ASSERT_TRUE(ledcAttachChannel(7, 1000, 8, 2));
  EXPECT_EQ(440U, ledcWriteTone(7, 440));
  EXPECT_EQ(128U, ledcRead(7));
  const auto tone = SquareSignal(7);
  EXPECT_DOUBLE_EQ(440.0, tone.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      0.5, std::get<roo_testing_transducers::ConstantDuty>(tone.duty).duty);
  EXPECT_EQ(440U, ledcWriteNote(7, NOTE_A, 4));

  ASSERT_TRUE(ledcOutputInvert(7, true));
  EXPECT_EQ(0U, ledcWriteTone(7, 0));
  const auto idle = FakeEsp32().gpio.get(7).lastSignal();
  ASSERT_TRUE(idle.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            idle->kind());
  EXPECT_FLOAT_EQ(roo_testing_transducers::VoltageDigitalHigh(),
                  idle->voltageAtUptimeMicros(0));
}

// Verifies configuration changes republish and detached/reused pins go low.
TEST_F(ArduinoLedcTest, RepublishesChangesAndDrivesDetachedAndReusedPinsLow) {
  ASSERT_TRUE(ledcAttachChannel(8, 1000, 8, 3));
  ASSERT_TRUE(ledcWriteChannel(3, 64));
  EXPECT_EQ(2000U, ledcChangeFrequency(8, 2000, 4));
  EXPECT_EQ(16U, ledcRead(8));
  const auto changed = SquareSignal(8);
  EXPECT_DOUBLE_EQ(2000.0, changed.carrier.frequency_hz);
  EXPECT_DOUBLE_EQ(
      1.0, std::get<roo_testing_transducers::ConstantDuty>(changed.duty).duty);
  ASSERT_TRUE(ledcOutputInvert(8, true));
  EXPECT_TRUE(SquareSignal(8).inverted);

  ASSERT_TRUE(ledcAttachChannel(9, 1000, 8, 4));
  ASSERT_TRUE(ledcWrite(9, 20));
  ASSERT_TRUE(ledcAttachChannel(10, 1000, 8, 4));
  EXPECT_EQ(0U, ledcRead(10));
  const auto old_pin = FakeEsp32().gpio.get(9).lastSignal();
  ASSERT_TRUE(old_pin.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            old_pin->kind());
  EXPECT_FLOAT_EQ(0.0f, old_pin->voltageAtUptimeMicros(0));
  ASSERT_TRUE(ledcDetach(10));
  const auto detached = FakeEsp32().gpio.get(10).lastSignal();
  ASSERT_TRUE(detached.has_value());
  EXPECT_EQ(roo_testing_transducers::VoltageSignalKind::kConstant,
            detached->kind());
  EXPECT_FLOAT_EQ(0.0f, detached->voltageAtUptimeMicros(0));
}

}  // namespace
