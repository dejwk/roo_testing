#pragma once

#include <string>

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeIli9486Spi : public SimpleFakeSpiDevice {
 public:
  FakeIli9486Spi(int cs, int dc, int rst,
                 roo_testing_transducers::Viewport& viewport,
                 const std::string& name = "display_ILI9486")
      : SimpleFakeSpiDevice(name, cs),
        dc_pin_(dc),
        rst_pin_(rst),
        pinDC_(name + ":DC"),
        pinRST_(name + ":RESET"),
        viewport_(viewport),
        mad_ctl_(0),
        is_reset_(true),
        is_inverted_(false),
        cmd_done_(true),
        buf_size_(0) {
    getGpioInterface()->attach(dc, pinDC_);
    getGpioInterface()->attach(rst, pinRST_);
    viewport_.init(320, 480);
  }

  ~FakeIli9486Spi() {
    getGpioInterface()->detach(dc_pin_);
    getGpioInterface()->detach(rst_pin_);
    SimpleFakeSpiDevice::~SimpleFakeSpiDevice();
  }

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
                uint16_t bit_count) override;

  void flush() override { viewport_.flush(); }

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

  void handleCmd(uint16_t cmd);
  void handleData();
  void reset();

  int16_t last_command_;

  void writeColor(uint16_t color);

  uint8_t dc_pin_;
  uint8_t rst_pin_;
  SimpleFakeGpioPin pinDC_;
  SimpleFakeGpioPin pinRST_;

  roo_testing_transducers::Viewport& viewport_;

  MadCtl mad_ctl_;

  int16_t x0_, y0_, x1_, y1_, x_cursor_, y_cursor_;
  bool is_reset_;
  bool is_inverted_;
  bool cmd_done_;
  uint16_t buf_[16];
  int buf_size_;
};
