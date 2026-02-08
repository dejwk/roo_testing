#include <gtest/gtest.h>

#include "roo_testing/buses/onewire/fake_onewire.h"
#include "roo_testing/devices/onewire/thermometer/thermometer.h"
#include "roo_testing/transducers/temperature/temperature.h"

using roo_testing_transducers::FixedThermometer;
using roo_testing_transducers::Temperature;

TEST(OneWireExampleTest, RegistersThermometersOnBus) {
  FixedThermometer t1(Temperature::FromC(25.0));
  FixedThermometer t2(Temperature::FromC(18.0));

  FakeOneWireInterface bus({
      new FakeOneWireThermometer("28884B9B0A0000D3", t1),
      new FakeOneWireThermometer("286E1D990A000046", t2),
  });

  FakeOneWireDevice::Rom rom1("28884B9B0A0000D3");
  FakeOneWireDevice::Rom rom2("286E1D990A000046");

  EXPECT_NE(bus.getDevice(rom1), nullptr);
  EXPECT_NE(bus.getDevice(rom2), nullptr);
}
