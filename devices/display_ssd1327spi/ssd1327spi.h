#pragma once

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/devices/roo_display/fltk_framebuffer.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(int cs, int dc, int rst, int magnification = 1,
                 Framebuffer::Rotation rotation = Framebuffer::kRotationNone)
      : SimpleFakeSpiDevice(cs),
        pinDC_(new FakeGpioPin()),
        pinRST_(new FakeGpioPin()),
        framebuffer_(128, 128, magnification, rotation) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
  }

  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override;

 private:
  FakeGpioPin* pinDC_;
  FakeGpioPin* pinRST_;

  Framebuffer framebuffer_;
};
