#pragma once

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/devices/roo_display/viewport.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(int cs, int dc, int rst, Viewport& viewport)
      : SimpleFakeSpiDevice(cs),
        pinDC_(new FakeGpioPin()),
        pinRST_(new FakeGpioPin()),
        viewport_(viewport) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
    viewport_.init(128, 128);
  }

  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override;

 private:
  void writeColor(uint8_t color);

  FakeGpioPin* pinDC_;
  FakeGpioPin* pinRST_;

  Viewport& viewport_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
};
