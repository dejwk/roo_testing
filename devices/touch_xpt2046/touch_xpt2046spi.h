#pragma once

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport.h"
#include <iostream>

class FakeXpt2046Spi : public SimpleFakeSpiDevice {
 public:
  FakeXpt2046Spi(int cs, Viewport& viewport)
      : SimpleFakeSpiDevice(cs), viewport_(viewport),
        conversion_requested_(false), conversion_bytes_returned_(0) {}

  void transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order, uint8_t* buf,
                uint16_t bit_count) override {
    for (int i = 0; i < (bit_count + 7) / 8; i++) {
      buf[i] = transfer_byte(buf[i]);
    }
  }

 private:
  uint8_t transfer_byte(uint8_t val) {
    if (conversion_requested_ && conversion_bytes_returned_ == 0) {
      ++conversion_bytes_returned_;
      // Ignore the operand in this case.
      return (conversion_ >> 8);
    }
    uint8_t result ;
    if (!conversion_requested_) {
      result = 0;
    } else if (conversion_bytes_returned_ == 1) {
      result = conversion_ & 0xFF;
      conversion_requested_ = false;
      conversion_bytes_returned_ = 0;
    }
    int16_t x;
    int16_t y;
    bool clicked = viewport_.isMouseClicked(&x, &y);
    switch (val) {
      case 0xD0: {
        conversion_requested_ = true;
        conversion_ = clicked ? ((4096LL * x) / viewport_.width()) << 3 : 0;
        break;
      }
      case 0x90: {
        conversion_requested_ = true;
        conversion_ = clicked ? ((4096LL * y) / viewport_.height()) << 3 : 0;
        break;
      }
      case 0xB1: {
        conversion_requested_ = true;
        conversion_ = clicked ? 1250 : 0;
        break;
      }
      case 0xC1: {
        conversion_requested_ = true;
        conversion_ = 0xFFF << 3;
        break;
      }
      case 0x91: {
        conversion_ = 0;
        break;
      }
    }
    return result;
  }

  Viewport& viewport_;
  bool conversion_requested_;
  uint8_t conversion_bytes_returned_;
  uint16_t conversion_;
};
