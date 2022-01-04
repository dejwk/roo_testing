#pragma once

#include "roo_testing/buses/onewire/fake_onewire.h"
#include "roo_testing/transducers/temperature/temperature.h"

class FakeOneWireThermometer : public FakeOneWireBaseDevice {
 public:
  enum Model {
    MODEL_DS18S20 = 0x10,
    MODEL_DS1820 = 0x10,
    MODEL_DS18B20 = 0x28,
    MODEL_DS1822 = 0x22,
    MODEL_DS28EA00 = 0x42
  };

  enum Resolution {
    RESOLUTION_9_BIT = 9,
    RESOLUTION_10_BIT = 10,
    RESOLUTION_11_BIT = 11,
    RESOLUTION_12_BIT = 12,
  };

  enum Power { POWER_SUPPLIED = 0, POWER_PARASITE = 1 };

  class EepromState {
   public:
    EepromState(int8_t t_h, int8_t t_l)
        : EepromState(t_h, t_l, RESOLUTION_12_BIT) {}
    EepromState(int8_t t_h, int8_t t_l, Resolution resolution);
    const uint8_t get(int index) const { return eeprom_[index]; }
    int8_t t_h() const { return eeprom_[0]; }
    int8_t t_l() const { return eeprom_[1]; }
    void set(int index, uint8_t value) { eeprom_[index] = value; }

   private:
    uint8_t eeprom_[3];
  };

  FakeOneWireThermometer(
      const char* rom,
      std::unique_ptr<const roo_testing_transducers::Thermometer> thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(FakeOneWireDevice::Rom(rom),
                               std::move(thermometer), power) {}

  FakeOneWireThermometer(
      FakeOneWireDevice::Rom rom,
      std::unique_ptr<const roo_testing_transducers::Thermometer> thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(rom, EepromState(85, 0), std::move(thermometer),
                               power) {}

  FakeOneWireThermometer(
      FakeOneWireDevice::Rom rom, EepromState eeprom,
      std::unique_ptr<const roo_testing_transducers::Thermometer> thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(rom, eeprom, thermometer.release(), true,
                               power) {}

  FakeOneWireThermometer(
      const char* rom, const roo_testing_transducers::Thermometer& thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(FakeOneWireDevice::Rom(rom), thermometer,
                               power) {}

  FakeOneWireThermometer(
      FakeOneWireDevice::Rom rom,
      const roo_testing_transducers::Thermometer& thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(rom, EepromState(85, 0), thermometer, power) {}

  FakeOneWireThermometer(
      FakeOneWireDevice::Rom rom, EepromState eeprom,
      const roo_testing_transducers::Thermometer& thermometer,
      Power power = POWER_SUPPLIED)
      : FakeOneWireThermometer(rom, eeprom, &thermometer, false, power) {}

  ~FakeOneWireThermometer() {
    if (owned_) delete (thermometer_);
  }

  Power power() const { return power_; }
  void setPower(Power power) { power_ = power; }

  void start_function_command() override;
  void write_function_bit(bool bit) override;
  bool read_function_bit() override;

 private:
  FakeOneWireThermometer(
      FakeOneWireDevice::Rom rom, EepromState eeprom,
      const roo_testing_transducers::Thermometer* thermometer, bool owned,
      Power power = POWER_SUPPLIED);

  inline bool has_configurable_resolution() const {
    return rom().model() != MODEL_DS18S20;
  }

  inline Resolution get_resolution() const {
    return has_configurable_resolution() ? Resolution((scratchpad_[4] >> 5) + 9)
                                         : RESOLUTION_12_BIT;
  }

  void check_finish_conversion();
  void recalculate_scratch_crc();

  const roo_testing_transducers::Thermometer* thermometer_;
  bool owned_;
  Power power_;

  uint8_t scratchpad_[9];
  EepromState eeprom_;

  bool conversion_in_progress_;
  int64_t conversion_start_;
  roo_testing_transducers::Temperature temperature_;
  bool is_alarming_;
};
