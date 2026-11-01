#pragma once
#include "roo_testing/devices/clock/internal/i2c_rtc.h"

/// Emulates RV-3028-C7 at I2C address 0x52, with 12/24-hour calendar, PORF, and
/// volatile backup configuration. Starts at 2000-01-01 with the chip's
/// time-loss/halt indication set. Alarm firing, calibration, EEPROM
/// programming, and electrical supplies are outside this calendar/control
/// model.
class FakeRv3028 : public roo_testing_rtc::I2cRtc {
 public:
  /// Borrows @p clock for deterministic time; null uses the emulator clock.
  explicit FakeRv3028(roo_testing_transducers::Clock* clock = nullptr);
};
