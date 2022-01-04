#include "fake_gpio.h"

#include <cstdlib>

#include "glog/logging.h"

using testing_transducers::Voltage;

FakeGpioInterface::FakeGpioInterface() {}

FakeGpioInterface::~FakeGpioInterface() {
  for (auto& pin : pins_) {
    if (pin.owned) delete pin.ptr;
  }
}

void FakeGpioInterface::attach(uint8_t pin, FakeGpioPin* fake) {
  attachInternal(pin, fake, false);
}

void FakeGpioInterface::attachInternal(uint8_t pin, FakeGpioPin* fake,
                                       bool owned) {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& val = pins_[pin];
  if (val.ptr != nullptr) {
    LOG(ERROR) << "GPIO conflict: attaching " << fake->name() << " on pin "
               << int(pin) << " which had " << pins_[pin].ptr->name()
               << " already attached";
    if (val.owned) delete val.ptr;
  }
  val.ptr = fake;
  val.owned = owned;
}

namespace {

class InputPin : public SimpleFakeGpioPin {
 public:
  InputPin(const Voltage* input, bool owned)
      : SimpleFakeGpioPin(input->name()), input_(input), owned_(owned) {}

  ~InputPin() {
    if (owned_) delete input_;
  }

  float read() const override { return input_->read(); }

  void onWrite(float voltage) override {
    LOG(ERROR) << "Writing to a voltage input " << name()
               << " is a no-op and probably a bug";
  }

 private:
  const Voltage* input_;
  bool owned_;
};

}  // namespace

void FakeGpioInterface::attachInput(uint8_t pin, const Voltage& input) {
  attachInternal(pin, new InputPin(&input, false), true);
}

void FakeGpioInterface::attachInput(uint8_t pin, std::unique_ptr<const Voltage> input) {
  attachInternal(pin, new InputPin(input.release(), true), true);
}

FakeGpioPin& FakeGpioInterface::get(uint8_t pin) const {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& result = pins_[pin];
  if (result.ptr == nullptr) {
    result.ptr = new FakeGpioPin();
    result.owned = true;
  }
  return *result.ptr;
}

extern "C" {

void gpioFakeWrite(uint8_t pin, float voltage) {
  getGpioInterface()->get(pin).write(voltage);
}

float gpioFakeRead(uint8_t pin) { return getGpioInterface()->get(pin).read(); }
}
