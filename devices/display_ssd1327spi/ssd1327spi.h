#pragma once

#include "roo_testing/buses/spi/fake_spi.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(FakeGpioPin& pin) : SimpleFakeSpiDevice(pin) {}
  
  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override;
};
