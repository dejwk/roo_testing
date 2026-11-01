#pragma once
#include "roo_testing/buses/i2c/fake_i2c.h"
#include "roo_testing/devices/clock/internal/rtc_registers.h"

namespace roo_testing_rtc {

/// Pin-routed I2C adapter for the independent register model. Public chip fakes
/// inherit diagnostics, clock injection, and sequential pointer behavior.
class I2cRtc : public FakeI2cDevice {
 public:
  /// Creates the named @p chip at @p address, borrowing @p clock or emulator
  /// time.
  I2cRtc(const char* name, uint8_t address, Chip chip,
         roo_testing_transducers::Clock* clock);

  /// Selects a register and writes sequential data. Invalid input is NACKed.
  Result write(const uint8_t* data, uint16_t size, bool stop,
               uint16_t timeout) override;

  /// Reads a latched calendar burst (live for RV-8803) with pointer advance.
  Result read(uint8_t* data, uint16_t size, bool stop,
              uint16_t timeout) override;

  /// Restores the model's initial registers and resets the address pointer.
  void reset();

  /// Raises the chip's time-loss indication without replacing its calendar.
  void simulateTimeLoss();

  /// Injects raw @p value at @p address, bypassing hardware write masks.
  /// Useful for invalid BCD, stopped-clock, and pending-event test scenarios.
  void injectRegister(uint8_t address, uint8_t value);

 protected:
  RtcRegisters registers_;

 private:
  uint8_t address_ = 0;
};
}  // namespace roo_testing_rtc
