#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeSt77xxSpi : public SimpleFakeSpiDevice {
 public:
  FakeSt77xxSpi(roo_testing_transducers::Viewport& viewport,
                int16_t width, int16_t height,
                const std::string& name = "display_ST7XX")
      : SimpleFakeSpiDevice(name),
        dc_(name + ":DC"),
        rst_(name + ":RESET"),
        viewport_(viewport),
        mad_ctl_(0),
        width_(width),
        height_(height),
        is_reset_(true),
        is_inverted_(false),
        cmd_done_(true),
        buf_size_(0) {
    viewport_.init(width, height);
  }

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                uint16_t bit_count) override;

  void flush() override { viewport_.flush(); }

  roo_testing_transducers::VoltageSink& dc() { return dc_; }
  roo_testing_transducers::VoltageSink& rst() { return rst_; }

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
    uint8_t val_;
  };

  void handleCmd(uint8_t cmd);
  void handleData();
  void reset();

  uint8_t last_command_;

  void writeColor(uint16_t color);

  roo_testing_transducers::SimpleDigitalSink dc_;
  roo_testing_transducers::SimpleDigitalSink rst_;

  roo_testing_transducers::Viewport& viewport_;

  MadCtl mad_ctl_;

  int16_t width_;
  int16_t height_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
  bool is_reset_;
  bool is_inverted_;
  bool cmd_done_;
  uint8_t buf_[16];
  int buf_size_;
};
