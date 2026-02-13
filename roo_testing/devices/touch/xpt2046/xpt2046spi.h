#pragma once

#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeXpt2046Spi : public SimpleFakeSpiDevice {
 public:
  struct Calibration {
    Calibration(int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax)
        : Calibration(xMin, yMin, xMax, yMax, false, false, false) {}

    Calibration(int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax,
                bool right_to_left, bool bottom_to_top, bool swap_xy)
        : xMin(xMin),
          yMin(yMin),
          xMax(xMax),
          yMax(yMax),
          right_to_left(right_to_left),
          bottom_to_top(bottom_to_top),
          swap_xy(swap_xy) {}

    Calibration() : Calibration(0, 0, 4095, 4095) {}

    void calibrate(int16_t& x, int16_t& y) {
      if (bottom_to_top) {
        y = 4095 - y;
      }
      if (right_to_left) {
        x = 4095 - x;
      }
      if (swap_xy) {
        std::swap(x, y);
      }
      x = xMin + (x * (xMax - xMin + 1) + 4095) / 4096;
      y = yMin + (y * (yMax - yMin + 1) + 4095) / 4096;
    }

    int16_t xMin;
    int16_t yMin;
    int16_t xMax;
    int16_t yMax;
    bool right_to_left;
    bool bottom_to_top;
    bool swap_xy;
  };

  FakeXpt2046Spi(roo_testing_transducers::Viewport& viewport,
                 Calibration calibration = Calibration(),
                 const std::string& name = "touch_XPT2046")
      : SimpleFakeSpiDevice(name),
        viewport_(viewport),
        calibration_(calibration),
        conversion_requested_(false),
        conversion_bytes_returned_(0) {}

  void transfer(const FakeSpiInterface& spi, uint8_t* buf,
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
    uint8_t result = 0;
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
    if (clicked) {
      x = ((4096LL * x) / viewport_.width());
      y = ((4096LL * y) / viewport_.height());
      calibration_.calibrate(x, y);
      x = 4095 - x;
      y = 4095 - y;
    }
    switch (val & 0xF0) {
      case 0xD0: {
        conversion_requested_ = true;
        conversion_ = clicked ? x << 3 : 0;
        break;
      }
      case 0x90: {
        conversion_requested_ = true;
        conversion_ = clicked ? y << 3 : 0;
        break;
      }
      case 0xB0: {
        conversion_requested_ = true;
        conversion_ = clicked ? 1250 << 3 : 0;
        break;
      }
      case 0xC0: {
        conversion_requested_ = true;
        conversion_ = 0xFFF << 3;
        break;
      }
    }
    return result;
  }

  roo_testing_transducers::Viewport& viewport_;
  Calibration calibration_;
  bool conversion_requested_;
  uint8_t conversion_bytes_returned_;
  uint16_t conversion_;
};
