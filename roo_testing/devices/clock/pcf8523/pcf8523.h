#pragma once
#include "roo_testing/devices/clock/internal/i2c_rtc.h"

/// Emulates PCF8523 at I2C address 0x68, with 12/24-hour calendar, STOP/OS
/// flags, and backup settings. Starts at 2000-01-01 with the chip's
/// time-loss/halt indication set. Alarm firing, calibration, EEPROM
/// programming, and electrical supplies are outside this calendar/control
/// model.
class FakePcf8523 : public roo_testing_rtc::I2cRtc {
 public:
  /// Borrows @p clock for deterministic time; null uses the emulator clock.
  explicit FakePcf8523(roo_testing_transducers::Clock* clock = nullptr);
};
