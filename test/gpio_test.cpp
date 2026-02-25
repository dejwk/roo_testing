#include <gtest/gtest.h>

#include "soc/gpio_struct.h"

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/transducers/voltage/voltage.h"

using namespace roo_testing_transducers;

TEST(GpioExampleTest, MirrorsOutputToInput) {
  ConstVoltage digital_input(0.0f);
  SimpleDigitalSink trigger(
      "trigger", [&](DigitalLevel level) { digital_input.set(level); });

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
