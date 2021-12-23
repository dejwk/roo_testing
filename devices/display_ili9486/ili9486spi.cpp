#include "roo_testing/devices/display_ili9486/ili9486spi.h"

#include <glog/logging.h>

#include <iostream>

namespace {

uint16_t read16(uint8_t*& buf) {
  uint16_t result = (buf[0] << 8) + buf[1];
  buf += 2;
  return result;
}

}  // namespace

void FakeIli9486Spi::transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order,
                              uint8_t* buf, uint16_t bit_count) {
  if (pinRST_->isDigitalLow()) return;
  bool command = pinDC_->isDigitalLow();
  if (command) {
    CHECK_EQ(16, bit_count);
    last_command_ = read16(buf);
    return;
  } else {
    switch (last_command_) {
      case 0x2A: {  // CASET
        CHECK_EQ(64, bit_count);
        x0_ = (read16(buf) << 8) + read16(buf);
        x1_ = (read16(buf) << 8) + read16(buf);
        x_cursor_ = x0_;
        y_cursor_ = y0_;
        break;
      }
      case 0x2B: {  // PASET
        CHECK_EQ(64, bit_count);
        y0_ = (read16(buf) << 8) + read16(buf);
        y1_ = (read16(buf) << 8) + read16(buf);
        x_cursor_ = x0_;
        y_cursor_ = y0_;
        break;
      }
      case 0x2C: {  // RAMWR
        for (int i = 0; i < bit_count / 16; ++i) {
          writeColor(read16(buf));
        }
        break;
      }
      case 0x36: {  // MADCTL
        CHECK_EQ(16, bit_count);
        mad_ctl_ = MadCtl(read16(buf));
        break;
      }
    }
  }
}

void FakeIli9486Spi::writeColor(uint16_t color) {
  uint32_t r = ((color >> 8) & 0xF8) | (color >> 13);
  uint32_t g = ((color >> 3) & 0xFC) | ((color >> 9) & 0x03);
  uint32_t b = ((color << 3) & 0xF8) | ((color >> 2) & 0x07);
  uint32_t argb = 0xFF000000 | (r << 16) | (g << 8) | b;

  uint16_t x = x_cursor_;
  uint16_t y = y_cursor_;

  if (mad_ctl_.mv()) {
    std::swap(x, y);
  }
  if (mad_ctl_.my()) {
    y = 479 - y;
  }
  if (!mad_ctl_.mx()) {
    x = 319 - x;
  }
  viewport_.fillRect(x, y, x, y, argb);
  if (x_cursor_ < x1_) {
    ++x_cursor_;
    return;
  }
  x_cursor_ = x0_;
  ++y_cursor_;
  return;
}
