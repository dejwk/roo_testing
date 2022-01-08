#pragma once

#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/buses/gpio/fake_gpio.h"

class FakeI2cPcf8574 : public FakeI2cDevice {
 public:
  FakeI2cPcf8574(uint8_t addr = 0);
  Result write(uint8_t *buff, uint16_t size, bool sendStop,
               uint16_t timeOutMillis) override;
  Result read(uint8_t *buff, uint16_t size, bool sendStop,
              uint16_t timeOutMillis) override;

  FakeGpioInterface gpio;
};
