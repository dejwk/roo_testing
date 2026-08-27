#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/voltage/voltage.h"
#include "soc/gpio_struct.h"

using namespace roo_testing_transducers;

namespace {

class RecordingVoltageIo : public VoltageIO {
 public:
  /// Returns the device name.
  const std::string& name() const override { return name_; }

  /// Returns the configured input voltage.
  float read() const override { return input_voltage_; }

  /// Retains a signal written through the GPIO adapter.
  void write(const VoltageSignal& signal) override { written_signal_ = signal; }

  /// Returns the signal most recently written to the device.
  const std::optional<VoltageSignal>& writtenSignal() const {
    return written_signal_;
  }

 private:
  std::string name_ = "recording_voltage_io";
  float input_voltage_ = 1.2f;
  std::optional<VoltageSignal> written_signal_;
};

}  // namespace

// Verifies scalar GPIO writes still mirror an output to an attached input.
TEST(GpioExampleTest, MirrorsOutputToInput) {
  ConstVoltage digital_input(0.0f);
  SimpleDigitalSink trigger = SimpleDigitalSink::WithSignalCallback(
      "trigger", [&](const VoltageSignal& signal) {
        digital_input.set(DigitalLevelFromVoltage(AverageDcVoltage(signal)));
      });

  FakeGpioInterface gpio(40);
  gpio.attachInput(33, digital_input);
  gpio.attachOutput(4, trigger);

  gpio.get(4).digitalWriteHigh();
  EXPECT_EQ(gpio.get(33).digitalRead(), kDigitalHigh);

  gpio.get(4).digitalWriteLow();
  EXPECT_EQ(gpio.get(33).digitalRead(), kDigitalLow);

  gpio.detach(33);
  gpio.detach(4);
}

// Verifies GPIO preserves signal descriptors while sampling their carrier.
TEST(FakeGpioPinTest, RetainsAndForwardsCompleteSignal) {
  const VoltageSignal pwm =
      VoltageSignal::Square(0, 3.3f, 1000, ConstantDuty{0.5}, 0);
  SimpleVoltageSink sink;
  FakeGpioInterface gpio(1);
  gpio.attachOutput(0, sink);

  gpio.get(0).write(pwm);

  ASSERT_TRUE(gpio.get(0).lastSignal().has_value());
  EXPECT_EQ(pwm, *gpio.get(0).lastSignal());
  ASSERT_TRUE(sink.signal().has_value());
  EXPECT_EQ(pwm, *sink.signal());
  EXPECT_FLOAT_EQ(3.3f, gpio.get(0).readAtUptimeMicros(0));
  EXPECT_FLOAT_EQ(0, gpio.get(0).readAtUptimeMicros(500));
  EXPECT_NEAR(1.65f, gpio.get(0).last_written(), 1e-6);
}

// Verifies GPIO routes input and output through a bidirectional voltage device.
TEST(FakeGpioInterfaceTest, RoutesVoltageIoAsInputAndOutput) {
  const VoltageSignal pwm =
      VoltageSignal::Square(0, 3.3f, 1000, ConstantDuty{0.25}, 0);
  RecordingVoltageIo io;
  FakeGpioInterface gpio(1);
  gpio.attach(0, io);

  EXPECT_FLOAT_EQ(1.2f, gpio.get(0).readAtUptimeMicros(999));
  gpio.get(0).write(pwm);

  ASSERT_TRUE(io.writtenSignal().has_value());
  EXPECT_EQ(pwm, *io.writtenSignal());
}

TEST(GpioRegisterInterceptTest, OutputSetClearAreInterceptedForLowBank) {
  constexpr int kPin = 4;

  SimpleDigitalSink sink("gpio4_sink");
  FakeEsp32().gpio.attachOutput(kPin, sink);

  GPIO.out_w1ts = (1UL << kPin);
  EXPECT_TRUE(sink.isHigh());

  GPIO.out_w1tc = (1UL << kPin);
  EXPECT_TRUE(sink.isLow());
}

TEST(GpioRegisterInterceptTest, OutputSetClearAreInterceptedForHighBank) {
  constexpr int kPin = 33;

  SimpleDigitalSink sink("gpio33_sink");
  FakeEsp32().gpio.attachOutput(kPin, sink);

  GPIO.out1_w1ts.val = (1UL << (kPin - 32));
  EXPECT_TRUE(sink.isHigh());

  GPIO.out1_w1tc.val = (1UL << (kPin - 32));
  EXPECT_TRUE(sink.isLow());
}

TEST(GpioRegisterInterceptTest, InputReadsComeFromAttachedSources) {
  constexpr int kLowBankPin = 2;
  constexpr int kHighBankPin = 34;

  ConstVoltage low_bank_src("low_bank", VoltageDigitalHigh());
  ConstVoltage high_bank_src("high_bank", VoltageDigitalLow());

  FakeEsp32().gpio.attachInput(kLowBankPin, low_bank_src);
  FakeEsp32().gpio.attachInput(kHighBankPin, high_bank_src);

  EXPECT_NE(0u, ((uint32_t)GPIO.in & (1UL << kLowBankPin)));

  uint32_t high_bank_bits = (uint32_t)GPIO.in1.val;
  EXPECT_EQ(0u, high_bank_bits & (1UL << (kHighBankPin - 32)));

  high_bank_src.setDigitalLow();
  low_bank_src.setDigitalLow();
  EXPECT_EQ(0u, ((uint32_t)GPIO.in & (1UL << kLowBankPin)));

  high_bank_src.set(kDigitalHigh);
  high_bank_bits = (uint32_t)GPIO.in1.val;
  EXPECT_NE(0u, high_bank_bits & (1UL << (kHighBankPin - 32)));
}
