#pragma once

#include "roo_testing/buses/spi/fake_spi.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(int cs, int dc, int rst)
      : SimpleFakeSpiDevice(cs),
        pinDC_(new FakeGpioPin()),
        pinRST_(new FakeGpioPin()) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
  }

  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override;

 private:
  FakeGpioPin* pinDC_;
  FakeGpioPin* pinRST_;
};
