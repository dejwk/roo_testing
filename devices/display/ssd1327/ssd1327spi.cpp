#include "roo_testing/devices/display/ssd1327/ssd1327spi.h"

void FakeSsd1327Spi::transfer(const FakeSpiInterface& spi, uint8_t* buf,
                              uint16_t bit_count) {
  rst_.warnIfUndef();
  dc_.warnIfUndef();
  if (rst_.isLow()) return;
  bool command = dc_.isLow();
  uint8_t* data = buf;
  uint8_t* end = buf + (bit_count + 7) / 8;
  if (command) {
    while (data < end) {
      uint8_t c = *data++;
      switch (c) {
        case 0x81:
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA8:
        case 0xB1:
        case 0xB3:
        case 0xAB:
        case 0xB6:
        case 0xBE:
        case 0xBC:
        case 0xD5:
        case 0xFD: {
          // Skip one operand.
          data++;
          break;
        }
        case 0xA4:
        case 0xAF: {
          // No operands.
          break;
        }
        case 0x15: {
          // CASET
          x0_ = 2 * *data++;
          x1_ = 2 * *data++ + 1;
          x_cursor_ = x0_;
          y_cursor_ = y0_;
          break;
        }
        case 0x75: {
          // RASET
          y0_ = *data++;
          y1_ = *data++;
          x_cursor_ = x0_;
          y_cursor_ = y0_;
          break;
        }
      }
    }
  } else {
    // Data. Always two pixel write.
    while (data < end) {
      uint8_t d = *data++;
      writeColor(d >> 4);
      writeColor(d & 0x0F);
    }
  }
}

void FakeSsd1327Spi::writeColor(uint8_t color) {
  viewport_.fillRect(x_cursor_, y_cursor_, x_cursor_, y_cursor_,
                     0xFF00000000 | color * 0x00111111);
  if (x_cursor_ < x1_) {
    ++x_cursor_;
    return;
  }
  x_cursor_ = x0_;
  ++y_cursor_;
  return;
}
