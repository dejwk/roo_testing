#pragma once

#include "roo_testing/buses/gpio/fake_gpio.h"
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/spi/fake_spi.h"

class FakeEsp32Board {
 public:
  FakeGpioInterface gpio;

  FakeI2cInterface i2c[2];
  FakeSpiInterface spi[4];

  FakeSpiInterface& fspi;  // References spi[1].
  FakeSpiInterface& hspi;  // References spi[2].
  FakeSpiInterface& vspi;  // References spi[3].

 private:
  friend FakeEsp32Board& FakeEsp32();

  FakeEsp32Board()
      : gpio(),
        i2c{FakeI2cInterface("i2c0"), FakeI2cInterface("i2c1")},
        spi{FakeSpiInterface("spi0(internal)"), FakeSpiInterface("spi1(FSPI)"),
            FakeSpiInterface("spi2(HSPI)"), FakeSpiInterface("spi3(VSPI)")},
        fspi(spi[1]),
        hspi(spi[2]),
        vspi(spi[3]) {}
};

FakeEsp32Board& FakeEsp32();
