#include <gtest/gtest.h>

#include "roo_testing/buses/gpio/fake_gpio.h"
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
