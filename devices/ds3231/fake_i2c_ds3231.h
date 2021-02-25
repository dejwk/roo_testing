#pragma once

#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/transducers/temperature/temperature.h"

class FakeI2cDs3231 : public FakeI2cDevice {
 public:
  enum HourMode {
    H24 = 0,
    H12 = 1,
  };

  FakeI2cDs3231();
  FakeI2cDs3231(testing_transducers::Thermometer *thermometer);
  Result write(uint8_t *buff, uint16_t size, bool sendStop,
               uint16_t timeOutMillis) override;
  Result read(uint8_t *buff, uint16_t size, bool sendStop,
              uint16_t timeOutMillis) override;

 private:
  void tick();
  void flush();
  void register_write(int index, uint8_t value);
  uint8_t register_read(int index) const;

  HourMode get_hour_mode() const {
    return (registers_[2] & 0x40) == 0 ? H24 : H12;
  }

  std::unique_ptr<testing_transducers::Thermometer> thermometer_;

  uint8_t register_address_;
  uint8_t registers_[0x12];
  bool registers_written_[0x12];

  uint64_t time_offset_;
  int weekday_offset_;
};
