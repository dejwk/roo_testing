#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport.h"

class FakeIli9486Spi : public SimpleFakeSpiDevice {
 public:
  FakeIli9486Spi(int cs, int dc, int rst, Viewport& viewport,
                 const std::string& name = "display_ILI9486")
      : SimpleFakeSpiDevice(name, cs),
        pinDC_(new FakeGpioPin()),
        pinRST_(new FakeGpioPin()),
        viewport_(viewport),
        mad_ctl_(0) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
    viewport_.init(320, 480);
  }

  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override;

 private:
  class MadCtl {
   public:
    MadCtl(uint16_t val) : val_(val) {}

    bool my() const { return val_ & 0x80; }
    bool mx() const { return val_ & 0x40; }
    bool mv() const { return val_ & 0x20; }
    bool ml() const { return val_ & 0x10; }
    bool bgr() const { return val_ & 0x08; }
    bool mh() const { return val_ & 0x04; }

   private:
    uint16_t val_;
  };

  int16_t last_command_;

  void writeColor(uint16_t color);

  FakeGpioPin* pinDC_;
  FakeGpioPin* pinRST_;

  Viewport& viewport_;

  MadCtl mad_ctl_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
};
