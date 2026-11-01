#pragma once

#include <stddef.h>
#include <stdint.h>

#include <array>

#include "roo_testing/transducers/time/clock.h"

namespace roo_testing_rtc {

/// Register maps shared by host device models, independently of roo_rtc.
enum class Chip {
  kDs3231,
  kDs3232,
  kDs3234,
  kDs1307,
  kPcf8523,
  kRv3028,
  kRv8803
};

/// Models calendar progression, halt/reset controls, validity and W0C flags.
/// The caller brackets latched I2C/SPI reads with beginRead()/endRead().
/// RV-8803 deliberately remains live during those reads.
class RtcRegisters {
 public:
  /// Creates the @p chip register bank, borrowing @p clock or emulator time.
  RtcRegisters(Chip chip, roo_testing_transducers::Clock* clock);

  /// Restores initial calendar/control registers and discards prior sampling.
  void reset();

  /// Returns the modeled sequential register-bank size, including RAM.
  uint16_t size() const;

  /// Advances @p address, wrapping at the end of the modeled bank.
  uint8_t nextAddress(uint8_t address) const;

  /// Captures a calendar snapshot for a bus frame; RV-8803 stays live.
  void beginRead();

  /// Releases the current calendar snapshot.
  void endRead();

  /// Samples @p address, applying aliases and the frame's latch semantics.
  uint8_t read(uint8_t address);

  /// Writes @p value at @p address, applying control and hardware status masks.
  void write(uint8_t address, uint8_t value);

  /// Raises the chip's time-loss/halt indication without changing the date.
  void simulateTimeLoss();

  /// Injects raw fault data bypassing masks; invalid calendars remain raw
  /// instead of being normalized into plausible dates by the model.
  void injectRegister(uint8_t address, uint8_t value);

  /// Updates DS323x temperature registers, clamping @p celsius to chip bounds.
  void setTemperature(double celsius);

 private:
  struct Calendar {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
  };

  uint8_t canonicalAddress(uint8_t address) const;
  int timeBase() const;
  bool isDs323x() const;
  bool stopped() const;
  bool twelveHour() const;
  bool decode(Calendar& calendar) const;
  void encode(const Calendar& calendar);
  void advance(uint64_t seconds);
  void sync();
  uint8_t statusWrite(uint8_t old, uint8_t value) const;

  Chip chip_;
  roo_testing_transducers::Clock* clock_;
  std::array<uint8_t, 256> data_ = {};
  std::array<uint8_t, 7> snapshot_ = {};
  int64_t sampled_at_ = 0;
  int64_t fraction_us_ = 0;
  bool sampled_ = false;
  bool reading_ = false;
};

}  // namespace roo_testing_rtc
