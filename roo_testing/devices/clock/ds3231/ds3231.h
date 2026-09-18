#pragma once

#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/transducers/temperature/temperature.h"

/// Emulates DS3231 timekeeping and its sequential register address pointer.
class FakeDs3231 : public FakeI2cDevice {
 public:
  enum HourMode {
    H24 = 0,
    H12 = 1,
  };

  /// Creates a clock with a fixed 25 degree Celsius temperature source.
  FakeDs3231();

  /// Takes ownership of the temperature source used for register readings.
  FakeDs3231(roo_testing_transducers::Thermometer* thermometer);

  /// Selects a register and writes sequential bytes, wrapping after 0x12.
  Result write(const uint8_t* buf, uint16_t size, bool sendStop,
               uint16_t timeOutMillis) override;

  /// Reads sequential bytes from the selected register, wrapping after 0x12.
  Result read(uint8_t* buff, uint16_t size, bool sendStop,
              uint16_t timeOutMillis) override;

 private:
  void tick();
  void flush();
  void register_write(int index, uint8_t value);
  uint8_t register_read(int index) const;

  HourMode get_hour_mode() const {
    return (registers_[2] & 0x40) == 0 ? H24 : H12;
  }

  std::unique_ptr<roo_testing_transducers::Thermometer> thermometer_;

  uint8_t register_address_;
  static constexpr uint8_t kRegisterCount = 0x13;
  uint8_t registers_[kRegisterCount];
  bool registers_written_[kRegisterCount];

  uint64_t time_offset_;
  int weekday_offset_;
};
