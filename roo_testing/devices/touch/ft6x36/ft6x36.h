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
             const std::string& name = "touch_FT6x36");

  Result write(const uint8_t* buf, uint16_t size, bool sendStop,
               uint16_t timeOutMillis) override;

  Result read(uint8_t* buff, uint16_t size, bool sendStop,
              uint16_t timeOutMillis) override;

 private:
  roo_testing_transducers::Viewport& viewport_;
  Calibration calibration_;
  int16_t x_, y_;
  bool pressed_;
};
