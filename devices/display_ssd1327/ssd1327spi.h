#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(int cs, int dc, int rst,
                 roo_testing_transducers::Viewport& viewport,
                 const std::string& name = "display_SSD1327")
      : SimpleFakeSpiDevice(name, cs),
        dc_pin_(dc),
        rst_pin_(rst),
        pinDC_(name + ":DC"),
        pinRST_(name + ":RESET"),
        viewport_(viewport) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
    viewport_.init(128, 128);
  }

  ~FakeSsd1327Spi() {
    getGpioInterface()->detach(dc_pin_);
    getGpioInterface()->detach(rst_pin_);
    SimpleFakeSpiDevice::~SimpleFakeSpiDevice();
  }

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                uint16_t bit_count) override;

  void flush() override { viewport_.flush(); }

 private:
  void writeColor(uint8_t color);

  uint8_t dc_pin_;
  uint8_t rst_pin_;
  SimpleFakeGpioPin pinDC_;
  SimpleFakeGpioPin pinRST_;

  roo_testing_transducers::Viewport& viewport_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
};
