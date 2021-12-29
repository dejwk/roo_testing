#include "fake_gpio.h"

#include <cstdlib>

FakeGpioInterface::FakeGpioInterface() {}

void FakeGpioInterface::attach(uint8_t pin, std::unique_ptr<FakeGpioPin> fake) {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  pins_[pin] = std::move(fake);
}

FakeGpioPin& FakeGpioInterface::get(uint8_t pin) const {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& result = pins_[pin];
  if (result == nullptr) {
    result.reset(new FakeGpioPin());
  }
  return *result;
}

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  getGpioInterface()->get(pin).write(voltage);
}

float gpioFakeRead(uint8_t pin) { return getGpioInterface()->get(pin).read(); }
}
