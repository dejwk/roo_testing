#pragma once
#include <memory>

#include "roo_testing/devices/clock/internal/i2c_rtc.h"
#include "roo_testing/transducers/temperature/temperature.h"

/// Emulates DS3231 calendar/control/status and sequential registers at 0x68.
/// Starts at 2000-01-01 with OSF set; set a trusted calendar and clear OSF to
/// make time valid. Models 12/24-hour time, W0C flags, and temperature
/// readings. Alarm firing, oscillator calibration, and electrical power are not
/// modeled.
class FakeDs3231 : public roo_testing_rtc::I2cRtc {
 public:
  /// Compatible names for the chip's two calendar hour encodings.
  enum HourMode { H24 = 0, H12 = 1 };

  /// Creates a clock using emulator time and a fixed 25 degree Celsius source.
  FakeDs3231();

  /// Borrows @p clock for deterministic time advancement at 25 degrees C.
  explicit FakeDs3231(roo_testing_transducers::Clock& clock);

  /// Takes ownership of @p thermometer and optionally borrows @p clock.
  /// A null clock selects emulator time; the thermometer must be non-null.
  explicit FakeDs3231(roo_testing_transducers::Thermometer* thermometer,
                      roo_testing_transducers::Clock* clock = nullptr);

  /// Selects/writes sequential registers, wrapping after 0x12.
  Result write(const uint8_t* data, uint16_t size, bool stop,
               uint16_t timeout) override;

  /// Reads sequential registers, latching calendar data per read burst.
  Result read(uint8_t* data, uint16_t size, bool stop,
              uint16_t timeout) override;

 private:
  std::unique_ptr<roo_testing_transducers::Thermometer> thermometer_;
};
