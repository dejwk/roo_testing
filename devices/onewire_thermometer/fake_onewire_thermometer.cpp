#include "fake_onewire_thermometer.h"
#include "roo_testing/transducers/time/clock.h"

#include <stdint.h>
#include <cmath>

inline int64_t getSystemTimeMicros() {
  return testing_transducers::getDefaultSystemClock()->getTimeMicros();
}

using testing_transducers::Temperature;

static inline void set_bit(uint8_t* buf, int idx, bool bit) {
  uint8_t mask = 1 << (idx & 0x7);
  if (bit) {
    buf[idx >> 3] |= mask;
  } else {
    buf[idx >> 3] &= ~mask;
  }
}

static inline bool get_bit(const uint8_t* buf, int idx) {
  return (buf[idx >> 3] >> (idx & 0x7)) & 1;
}

enum Command {
  CONVERT_T = 0x44,
  READ_SCRATCHPAD = 0xBE,
  WRITE_SCRATCHPAD = 0x4E,
  COPY_SCRACTHPAD = 0x48,
  RECALL_E2 = 0xB8,
  READ_POWER_SUPPLY = 0xB4
};

FakeOneWireThermometer::EepromState::EepromState(int8_t t_h, int8_t t_l,
                                                 Resolution resolution) {
  eeprom_[0] = t_h;
  eeprom_[1] = t_l;
  eeprom_[2] = (((resolution - 9) & 0x3) << 5) | 0x1F;
}

FakeOneWireThermometer::FakeOneWireThermometer(
    FakeOneWireDevice::Rom rom, EepromState eeprom,
    std::unique_ptr<testing_transducers::Thermometer> thermometer, Power power)
    : FakeOneWireBaseDevice(rom),
      thermometer_(std::move(thermometer)),
      power_(power),
      eeprom_(eeprom),
      conversion_in_progress_(false),
      conversion_start_(0),
      temperature_(Temperature::FromC(0)),
      is_alarming_(false) {
  scratchpad_[0] = 0xAA;
  scratchpad_[1] = 0x00;
  scratchpad_[2] = eeprom_.get(0);
  scratchpad_[3] = eeprom_.get(1);
  scratchpad_[4] = 0xFF;
  scratchpad_[5] = 0xFF;
  scratchpad_[6] = 0x0C;
  scratchpad_[7] = 0x10;
  recalculate_scratch_crc();
}

inline uint8_t reverse_byte(uint8_t a) {
  return ((a & 0x1) << 7) | ((a & 0x2) << 5) | ((a & 0x4) << 3) |
         ((a & 0x8) << 1) | ((a & 0x10) >> 1) | ((a & 0x20) >> 3) |
         ((a & 0x40) >> 5) | ((a & 0x80) >> 7);
}

void FakeOneWireThermometer::recalculate_scratch_crc() {
  scratchpad_[8] = onewire_crc(scratchpad_, 8);
}

void FakeOneWireThermometer::start_function_command() {
  check_finish_conversion();
  switch (function_command()) {
    case CONVERT_T: {
      temperature_ = thermometer_->Read();
      conversion_in_progress_ = true;
      conversion_start_ = getSystemTimeMicros();
      break;
    }
    case COPY_SCRACTHPAD: {
      eeprom_.set(0, scratchpad_[2]);
      eeprom_.set(1, scratchpad_[3]);
      if (has_configurable_resolution()) {
        eeprom_.set(2, scratchpad_[4]);
      }
      break;
    }
    case RECALL_E2: {
      scratchpad_[2] = eeprom_.get(0);
      scratchpad_[3] = eeprom_.get(1);
      if (has_configurable_resolution()) {
        scratchpad_[4] = eeprom_.get(2);
      }
      recalculate_scratch_crc();
      break;
    }
  }
}

static const int64_t getTconvUs(FakeOneWireThermometer::Resolution resolution) {
  switch (resolution) {
    case FakeOneWireThermometer::RESOLUTION_9_BIT:
      return 90000;
    case FakeOneWireThermometer::RESOLUTION_10_BIT:
      return 180000;
    case FakeOneWireThermometer::RESOLUTION_11_BIT:
      return 360000;
    default:
      return 720000;
  }
}

void FakeOneWireThermometer::check_finish_conversion() {
  if (conversion_in_progress_ && (getSystemTimeMicros() - conversion_start_) >=
                                     getTconvUs(get_resolution())) {
    conversion_in_progress_ = false;
    conversion_start_ = 0;
    double tempC = temperature_.AsC();
    if (tempC < -55.0) {
      tempC = -55.0;
    } else if (tempC > 125.0) {
      tempC = 125.0;
    }
    if (has_configurable_resolution()) {
      int16_t temp = (int16_t)(tempC * 16);
      scratchpad_[0] = temp & 0xFF;
      scratchpad_[1] = (temp >> 8) & 0xFF;
    } else {
      scratchpad_[0] = (uint8_t)(tempC * 2);
      scratchpad_[1] = (tempC >= 0 ? 0x00 : 0xFF);
      double ignored;
      scratchpad_[6] = ((modf(tempC, &ignored)) * 16.0);
    }
    recalculate_scratch_crc();

    int8_t alarming_temp = (int8_t)tempC;
    is_alarming_ = (alarming_temp >= (int8_t)scratchpad_[2] ||
                    alarming_temp <= (int8_t)scratchpad_[3]);
  }
}

void FakeOneWireThermometer::write_function_bit(bool bit) {
  switch (function_command()) {
    case WRITE_SCRATCHPAD: {
      set_bit(scratchpad_ + 2, bits_received(), bit);
      recalculate_scratch_crc();
      break;
    }
  }
}

bool FakeOneWireThermometer::read_function_bit() {
  switch (function_command()) {
    case READ_SCRATCHPAD: {
      return get_bit(scratchpad_, bits_sent());
    }
    case READ_POWER_SUPPLY: {
      return (power_ == POWER_SUPPLIED);
    }
    case CONVERT_T: {
      if (power_ == POWER_SUPPLIED) {
        check_finish_conversion();
        return !conversion_in_progress_;
      }
    }
  }
  return true;
}
