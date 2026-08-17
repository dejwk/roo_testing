#include "esp32-hal-adc.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-periman.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {
uint8_t read_width = 12;
uint8_t hardware_width = 12;
adc_attenuation_t default_attenuation = ADC_11db;
std::array<int8_t, SOC_GPIO_PIN_COUNT> pin_attenuation = [] {
  std::array<int8_t, SOC_GPIO_PIN_COUNT> value{};
  value.fill(-1);
  return value;
}();

uint16_t scale(int value) {
  value = std::max(0, value);
  if (read_width < hardware_width) value >>= hardware_width - read_width;
  if (read_width > hardware_width) value <<= read_width - hardware_width;
  return static_cast<uint16_t>(value);
}
}  // namespace

extern "C" {

uint16_t analogRead(uint8_t pin) {
  const int8_t analog_channel = digitalPinToAnalogChannel(pin);
  if (analog_channel < 0) return 0;
  const int unit = analog_channel >= 10 ? 1 : 0;
  const int channel = analog_channel >= 10 ? analog_channel - 10 : analog_channel;
  if (pin_attenuation[pin] < 0) {
    FakeEsp32().adc(unit).setAttenuation(channel, default_attenuation);
  }
  perimanSetPinBus(pin, ESP32_BUS_TYPE_ADC_ONESHOT, &FakeEsp32().adc(unit), unit, channel);
  return scale(FakeEsp32().adc(unit).convert(channel));
}

uint32_t analogReadMilliVolts(uint8_t pin) {
  const uint32_t maximum = (1u << read_width) - 1;
  if (!maximum) return 0;
  const double raw = static_cast<double>(analogRead(pin)) * 4095.0 / maximum;
  if (raw <= 0) return 0;
  const int8_t analog_channel = digitalPinToAnalogChannel(pin);
  if (analog_channel < 0) return 0;
  const int unit = analog_channel >= 10 ? 1 : 0;
  const int channel = analog_channel >= 10 ? analog_channel - 10 : analog_channel;
  const uint8_t attenuation = FakeEsp32().adc(unit).getAttenuation(channel);
  double volts = 0;
  switch (attenuation) {
    case ADC_0db: volts = raw >= 4095 ? 1.03 : raw / 4243.5 + 0.065; break;
    case ADC_2_5db: volts = raw >= 4095 ? 1.36 : raw / 3169.5 + 0.068; break;
    case ADC_6db: volts = raw >= 4095 ? 1.88 : raw / 2291.5 + 0.093; break;
    default: {
      if (raw >= 4095) {
        volts = 3.12;
      } else {
        constexpr double base = (2.5 - 0.13) * 1233.4;
        if (raw <= base) {
          volts = raw / 1233.4 + 0.13;
        } else {
          const double x = (-1233.4 + std::sqrt(1233.4 * 1233.4 +
                              4.0 * 1059.0 * (raw - base))) / (2.0 * 1059.0);
          volts = 2.5 + x;
        }
      }
    }
  }
  return static_cast<uint32_t>(volts * 1000.0 + 0.5);
}

void analogReadResolution(uint8_t bits) {
  if (bits >= 1 && bits <= 16) read_width = bits;
}

void analogSetAttenuation(adc_attenuation_t attenuation) {
  default_attenuation = attenuation;
}

void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t attenuation) {
  int8_t analog_channel = digitalPinToAnalogChannel(pin);
  if (analog_channel < 0) return;
  const int unit = analog_channel >= 10 ? 1 : 0;
  FakeEsp32().adc(unit).setAttenuation(analog_channel % 10, attenuation);
  pin_attenuation[pin] = attenuation;
}

void analogSetWidth(uint8_t bits) {
  hardware_width = std::min<uint8_t>(12, std::max<uint8_t>(9, bits));
  FakeEsp32().adc(0).setWidth(hardware_width - 9);
  FakeEsp32().adc(1).setWidth(hardware_width - 9);
}

bool analogContinuous(const uint8_t[], size_t, uint32_t, uint32_t, void (*)(void)) { return false; }
bool analogContinuousRead(adc_continuous_result_t **, uint32_t) { return false; }
bool analogContinuousStart() { return false; }
bool analogContinuousStop() { return true; }
bool analogContinuousDeinit() { return true; }
void analogContinuousSetAtten(adc_attenuation_t attenuation) { default_attenuation = attenuation; }
void analogContinuousSetWidth(uint8_t bits) { analogSetWidth(bits); }

}  // extern "C"
