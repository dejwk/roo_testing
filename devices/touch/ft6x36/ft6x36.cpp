#include "roo_testing/devices/touch/ft6x36/ft6x36.h"

#include "glog/logging.h"

FakeFt6x36::FakeFt6x36(roo_testing_transducers::Viewport& viewport,
                       Calibration calibration, const std::string& name)
    : FakeI2cDevice(name, 0x38),
      viewport_(viewport),
      calibration_(calibration),
      pressed_(false) {}

FakeFt6x36::Result FakeFt6x36::write(const uint8_t* buf, uint16_t size,
                                     bool sendStop,
                                     uint16_t timeOutMillis) {
  if (size == 1 && sendStop && buf[0] == 0) {
    return I2C_ERROR_OK;
  }
  LOG(WARNING) << "FT6x36 write: " << size << ", " << sendStop << ", "
               << timeOutMillis;
  return I2C_ERROR_DEV;
}

FakeFt6x36::Result FakeFt6x36::read(uint8_t* buff, uint16_t size, bool sendStop,
                                    uint16_t timeOutMillis) {
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
