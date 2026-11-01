#pragma once
#include "roo_testing/devices/clock/internal/i2c_rtc.h"

/// Emulates RV-8803-C7 at I2C address 0x32, with live calendar reads, one-hot
/// weekday, RESET, and voltage flags. Starts at 2000-01-01 with the chip's
/// time-loss/halt indication set. Alarm firing, calibration, EEPROM
/// programming, and electrical supplies are outside this calendar/control
/// model.
class FakeRv8803 : public roo_testing_rtc::I2cRtc {
 public:
  /// Borrows @p clock for deterministic time; null uses the emulator clock.
  explicit FakeRv8803(roo_testing_transducers::Clock* clock = nullptr);
};
