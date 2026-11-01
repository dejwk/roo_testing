#pragma once
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/devices/clock/internal/rtc_registers.h"

/// Emulates the DS3234 calendar/control/status protocol over active-low CS.
/// Supports modes 1/3, MSB-first, up to 4 MHz. A command plus consecutive data
/// can span transfer() calls while CS stays low. Starts with OSF set.
/// Alarm firing, calibration, SRAM's indirect port and supply voltages are
/// outside this model. Attach cs() to the board's chip-select output.
class FakeDs3234 : public SimpleFakeSpiDevice {
 public:
  /// Borrows @p clock for deterministic time; null uses emulator time.
  explicit FakeDs3234(roo_testing_transducers::Clock* clock = nullptr);

  /// Processes whole bytes of an asserted CS frame; invalid settings/commands
  /// return 0xff and latch protocolError() until clearProtocolError().
  void transfer(const FakeSpiInterface& spi, uint8_t* buffer,
                uint16_t bit_count) override;

  /// Restores initial registers and clears frame/protocol diagnostics.
  void reset();

  /// Raises the oscillator-stop indication without replacing the calendar.
  void simulateTimeLoss();

  /// Injects raw @p value at @p address, bypassing hardware write masks.
  void injectRegister(uint8_t address, uint8_t value);

  /// Reports unsupported SPI settings, commands, or partial-byte transfers.
  bool protocolError() const { return protocol_error_; }

  /// Clears protocol diagnostics, leaving clock registers unchanged.
  void clearProtocolError() { protocol_error_ = false; }

 protected:
  void onChipSelect(bool selected) override;

 private:
  roo_testing_rtc::RtcRegisters registers_;
  bool need_command_ = true;
  bool writing_ = false;
  bool frame_valid_ = true;
  bool protocol_error_ = false;
  uint8_t address_ = 0;
};
