#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeSsd1327Spi : public SimpleFakeSpiDevice {
 public:
  FakeSsd1327Spi(roo_testing_transducers::Viewport& viewport,
                 const std::string& name = "display_SSD1327")
      : SimpleFakeSpiDevice(name, cs),
        dc_(name + ":DC"),
        rst_(name + ":RESET"),
        viewport_(viewport) {
    viewport_.init(128, 128);
  }

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                uint16_t bit_count) override;

  void flush() override { viewport_.flush(); }

  roo_testing_transducers::VoltageSink& dc() { return dc_; }
  roo_testing_transducers::VoltageSink& rst() { return rst_; }

 private:
  void writeColor(uint8_t color);

  roo_testing_transducers::SimpleDigitalSink dc_;
  roo_testing_transducers::SimpleDigitalSink rst_;

  roo_testing_transducers::Viewport& viewport_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
};
