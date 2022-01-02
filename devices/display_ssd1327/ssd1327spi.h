#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(int cs, int dc, int rst, Viewport& viewport,
                 const std::string& name = "display_SSD1327")
      : SimpleFakeSpiDevice(name, cs),
        pinDC_(new SimpleFakeGpioPin(name + ":DC")),
        pinRST_(new SimpleFakeGpioPin(name + ":RESET")),
        viewport_(viewport) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
    viewport_.init(128, 128);
  }

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                uint16_t bit_count) override;

 private:
  void writeColor(uint8_t color);

  FakeGpioPin* pinDC_;
  FakeGpioPin* pinRST_;

  Viewport& viewport_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
};
