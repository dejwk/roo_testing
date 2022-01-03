#include "fake_gpio.h"

#include <cstdlib>

#include "glog/logging.h"

FakeGpioInterface::FakeGpioInterface() {}

void FakeGpioInterface::attach(uint8_t pin, FakeGpioPin* fake) {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  if (pins_[pin] != nullptr) {
    LOG(ERROR) << "GPIO conflict: attaching " << fake->name() << " on pin "
               << int(pin) << " which had " << pins_[pin]->name()
               << " already attached";
  }
  pins_[pin] = fake;
}

FakeGpioPin& FakeGpioInterface::get(uint8_t pin) const {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& result = pins_[pin];
  if (result == nullptr) {
    result = new FakeGpioPin();
  }
  return *result;
}

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  getGpioInterface()->get(pin).write(voltage);
}

float gpioFakeRead(uint8_t pin) { return getGpioInterface()->get(pin).read(); }
}
