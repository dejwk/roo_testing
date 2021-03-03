#include "fake_gpio.h"

#include <cstdlib>

class FloatingGpioPin : public FakeGpioPin {
 public:
  float read() override { return (float)rand() * 5.0 / RAND_MAX; }
  void write(float voltage) override {}
};

FakeGpioInterface::FakeGpioInterface() : default_(new FloatingGpioPin()) {}

void FakeGpioInterface::attach(uint8_t pin,
                               std::unique_ptr<FakeGpioPin> fake) {
  pins_.insert(std::make_pair(pin, std::move(fake)));
}

FakeGpioInterface* getGpioInterface() {
  static FakeGpioInterface gpio;
  return &gpio;
}

FakeGpioPin* FakeGpioInterface::get(uint8_t pin) const {
  auto i = pins_.find(pin);
  return (i == pins_.end()) ? default_.get() : i->second.get();
}

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  FakeGpioPin* fake = getGpioInterface()->get(pin);
  fake->write(voltage);
}

float gpioFakeRead(uint8_t pin) {
  FakeGpioPin* fake = getGpioInterface()->get(pin);
  return fake->read();
}

}
