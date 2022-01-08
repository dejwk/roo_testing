#include "roo_testing/devices/display/ili9486/ili9486spi.h"

#include <glog/logging.h>

namespace {

uint16_t read16(uint8_t*& buf) {
  uint16_t result = (buf[0] << 8) + buf[1];
  buf += 2;
  return result;
}

}  // namespace

void FakeIli9486Spi::transfer(const FakeSpiInterface& spi, uint8_t* buf,
                              uint16_t bit_count) {
  rst_.warnIfUndef();
  dc_.warnIfUndef();
  if (rst_.isLow()) {
    reset();
    return;
  }
  bool command = dc_.isLow();
  if (has_half_word_) {
    CHECK_GE(8, bit_count) << "The bit count is not divisible by 8: "
                           << bit_count;
    uint16_t val = (half_word_ << 8) + *buf++;
    has_half_word_ = false;
    if (command) {
      handleCmd(val & 0xFF);
    } else {
      buf_[buf_size_++] = val;
      handleData();
    }
    // buf++;
    bit_count -= 8;
  }
  if (command) {
    while (bit_count >= 16) {
      handleCmd(read16(buf));
      bit_count -= 16;
    }
  } else {
    while (bit_count >= 16) {
      buf_[buf_size_++] = read16(buf);
      handleData();
      bit_count -= 16;
    }
  }
  if (bit_count >= 8) {
    has_half_word_ = true;
    half_word_ = *buf++;
    bit_count -= 8;
  }
  CHECK_EQ(0, bit_count)
      << "The count of cmd/data bits must be a multiple of 8; " << bit_count
      << " bits remained.";
}

void FakeIli9486Spi::handleCmd(uint16_t cmd) {
  if (!cmd_done_) {
    LOG_EVERY_N(WARNING, 100)
        << std::hex << "Warning: new command 0x" << cmd
        << " has been received before the previous command 0x" << last_command_
        << " has completed.";
  }
  cmd_done_ = false;
  buf_size_ = 0;
  last_command_ = cmd;
  // Handle commands that do not expect any operands.
  switch (cmd) {
    case 0x00: {  // NOP.
      cmd_done_ = true;
      break;
    }
    case 0x01: {  // SWRESET.
      reset();
      cmd_done_ = true;
      break;
    }
    case 0x10: {  // SLPIN.
      LOG(WARNING) << "Received SLPIN, which isn't currently supported. "
                      "Ignoring.";
      cmd_done_ = true;
      break;
    }
    case 0x11: {  // SLPOUT.
      cmd_done_ = true;
      break;
    }
    case 0x20: {  // INVOFF.
      is_inverted_ = false;
      cmd_done_ = true;
    }
    case 0x28: {  // DISPOFF.
      LOG(WARNING) << "Received DISPOFF, which isn't currently supported. "
                      "Ignoring.";
      cmd_done_ = true;
      break;
    }
    case 0x29: {  // DISPON.
      cmd_done_ = true;
      break;
    }
    case 0x2C: {  // RAMWR.
      x_cursor_ = x0_;
      y_cursor_ = y0_;
      cmd_done_ = true;
      break;
    }
    default: {
      break;
    }
  }
}

void FakeIli9486Spi::handleData() {
  switch (last_command_) {
    case 0x2A: {  // CASET
      if (buf_size_ < 4) return;
      x0_ = ((buf_[0] & 0xFF) << 8) + (buf_[1] & 0xFF);
      x1_ = ((buf_[2] & 0xFF) << 8) + (buf_[3] & 0xFF);
      cmd_done_ = true;
      break;
    }
    case 0x2B: {  // PASET
      if (buf_size_ < 4) return;
      y0_ = ((buf_[0] & 0xFF) << 8) + (buf_[1] & 0xFF);
      y1_ = ((buf_[2] & 0xFF) << 8) + (buf_[3] & 0xFF);
      cmd_done_ = true;
      break;
    }
    case 0x2C: {  // RAMWR
      writeColor(buf_[0]);
      buf_size_ = 0;
      break;
    }
    case 0x36: {  // MADCTL
      mad_ctl_ = MadCtl(buf_[0]);
      cmd_done_ = true;
      break;
    }
    case 0x3A: {  // PIXSET
      if (buf_[0] != 0x55) {
        LOG(WARNING) << "Received an unsupported value of PIXSET: 0x"
                     << std::hex << buf_[0] << ". Ignoring.";
      }
      cmd_done_ = true;
      break;
    }
    case 0xC0:
    case 0xC1:
    case 0xC2:
    case 0xC5:
    case 0xC7:
    case 0xE0:
    case 0xE1: {  // PWCTRL*, VMCTRL*, NGAMCTRL
      // Silently ignore.
      buf_size_ = 0;
      cmd_done_ = true;
      break;
    }
    default: {
      // Silently ignore.
      buf_size_ = 0;
      cmd_done_ = true;
      break;
    }
  }
}

void FakeIli9486Spi::reset() {
  if (is_reset_) return;
  mad_ctl_ = 0;
  for (int i = 0; i < 480; i += 2) {
    viewport_.fillRect(0, i, 319, i, 0xFFCCCCCC);
    viewport_.fillRect(0, i + 1, 319, i + 1, 0xFF444444);
  }
  buf_size_ = 0;
  is_reset_ = true;
  is_inverted_ = false;
  has_half_word_ = false;
}

void FakeIli9486Spi::writeColor(uint16_t color) {
  uint32_t r = ((color >> 8) & 0xF8) | (color >> 13);
  uint32_t g = ((color >> 3) & 0xFC) | ((color >> 9) & 0x03);
  uint32_t b = ((color << 3) & 0xF8) | ((color >> 2) & 0x07);
  uint32_t argb = 0xFF000000 | (r << 16) | (g << 8) | b;
  if (is_inverted_) argb ^= 0x00FFFFFF;

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
