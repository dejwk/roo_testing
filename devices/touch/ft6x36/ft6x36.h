#pragma once

#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

class FakeFt6x36 : public FakeI2cDevice {
 public:
  enum Kind {
    PRESS_DOWN = 0x00 << 6,
    LIFT_UP = 0x01 << 6,
    CONTACT = 0x02 << 6,
    NO_EVENT = 0x03 << 6
  };

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

  FakeFt6x36(roo_testing_transducers::Viewport& viewport,
             Calibration calibration = Calibration(),
             const std::string& name = "touch_FT6x36")
      : FakeI2cDevice(name, 0x38),
        viewport_(viewport),
        calibration_(calibration),
        pressed_(false) {}

  Result write(const uint8_t* buf, uint16_t size, bool sendStop,
               uint16_t timeOutMillis) override {
    if (size == 1 && sendStop && buf[0] == 0) {
      return I2C_ERROR_OK;
    }
    LOG(WARNING) << "FT6x36 write: " << size << ", " << sendStop << ", "
                 << timeOutMillis;
    return I2C_ERROR_DEV;
  }

  Result read(uint8_t* buff, uint16_t size, bool sendStop,
              uint16_t timeOutMillis) override {
    if (size != 16 || !sendStop) {
      LOG(WARNING) << "FT6x36 read: " << size << ", " << sendStop;
      return I2C_ERROR_DEV;
    }
    buff[0] = 0;
    buff[1] = 0;

    int16_t new_x, new_y;
    bool new_pressed = viewport_.isMouseClicked(&new_x, &new_y);
    if (new_pressed) {
      new_x = ((4096LL * new_x) / viewport_.width());
      new_y = ((4096LL * new_y) / viewport_.height());
      calibration_.calibrate(new_x, new_y);
    }
    if (!pressed_ && !new_pressed) {
      buff[2] = 0;
      return I2C_ERROR_OK;
    }
    buff[2] = 1;
    Kind kind;
    if (!pressed_) {
      // Must be newly preset.
      kind = PRESS_DOWN;
    } else if (new_pressed) {
      kind = CONTACT;
    } else {
      kind = LIFT_UP;
    }
    if (new_pressed) {
      x_ = new_x;
      y_ = new_y;
    }
    buff[3] = ((x_ >> 8) & 0x3F) + kind;
    buff[4] = (x_ & 0xFF);
    buff[5] = ((y_ >> 8) & 0x0F) + 0x00;
    buff[6] = (y_ & 0xFF);
    buff[7] = 0xC0;

    return I2C_ERROR_OK;
  }

 private:
  roo_testing_transducers::Viewport& viewport_;
  Calibration calibration_;
  int16_t x_, y_;
  bool pressed_;
};
