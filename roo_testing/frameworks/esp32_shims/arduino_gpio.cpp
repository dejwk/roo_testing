#include "esp32-hal-gpio.h"
#include "esp32-hal-matrix.h"
#include "esp32-hal-periman.h"

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {
constexpr int8_t kAdcPins[] = {
    36, 37, 38, 39, 32, 33, 34, 35, -1, -1,
    4,  0,  2,  15, 13, 12, 14, 27, 25, 26,
};
}  // namespace

extern "C" {

void pinMode(uint8_t pin, uint8_t) {
  if (perimanPinIsValid(pin)) {
    perimanSetPinBus(pin, ESP32_BUS_TYPE_GPIO, nullptr, -1, -1);
  }
}

void digitalWrite(uint8_t pin, uint8_t value) {
  if (pin < SOC_GPIO_PIN_COUNT) {
    FakeEsp32().gpio.get(pin).digitalWrite(value ? roo_testing_transducers::kDigitalHigh
                                                 : roo_testing_transducers::kDigitalLow);
  }
}

int digitalRead(uint8_t pin) {
  return pin < SOC_GPIO_PIN_COUNT && FakeEsp32().gpio.get(pin).isDigitalHigh();
}

void attachInterrupt(uint8_t, void (*)(void), int) {}
void attachInterruptArg(uint8_t, void (*)(void *), void *, int) {}
void detachInterrupt(uint8_t) {}
void enableInterrupt(uint8_t) {}
void disableInterrupt(uint8_t) {}

int8_t digitalPinToTouchChannel(uint8_t pin) {
  static constexpr int8_t kTouchPins[] = {4, 0, 2, 15, 13, 12, 14, 27, 33, 32};
  for (int8_t channel = 0; channel < 10; ++channel) {
    if (kTouchPins[channel] == pin) return channel;
  }
  return -1;
}

int8_t digitalPinToAnalogChannel(uint8_t pin) {
  for (int8_t channel = 0; channel < static_cast<int8_t>(sizeof(kAdcPins)); ++channel) {
    if (kAdcPins[channel] == pin) return channel;
  }
  return -1;
}

int8_t analogChannelToDigitalPin(uint8_t channel) {
  return channel < sizeof(kAdcPins) ? kAdcPins[channel] : -1;
}

void pinMatrixOutAttach(uint8_t pin, uint8_t function, bool invert_out,
                        bool invert_enable) {
  if (pin < SOC_GPIO_PIN_COUNT) {
    FakeEsp32().out_matrix.assign(pin, function, invert_out, invert_enable);
  }
}

void pinMatrixOutDetach(uint8_t pin, bool invert_out, bool invert_enable) {
  if (pin < SOC_GPIO_PIN_COUNT) {
    FakeEsp32().out_matrix.assign(pin, kMatrixDetachOutSig, invert_out, invert_enable);
  }
}

void pinMatrixInAttach(uint8_t pin, uint8_t signal, bool inverted) {
  FakeEsp32().in_matrix.assign(pin, signal, inverted);
}

void pinMatrixInDetach(uint8_t signal, bool high, bool inverted) {
  FakeEsp32().in_matrix.assign(high ? kMatrixDetachInHighPin : kMatrixDetachInLowPin,
                               signal, inverted);
}

}  // extern "C"
