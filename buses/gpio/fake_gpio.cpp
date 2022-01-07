#include "fake_gpio.h"

#include <cstdlib>

#include "glog/logging.h"

using namespace roo_testing_transducers;

namespace {

// Adapter from pure output.
class Output : public FakeGpioPin {
 public:
  Output(VoltageSink* output, bool owned)
      : FakeGpioPin(), output_(output), owned_(owned) {}

  ~Output() {
    if (owned_) delete output_;
  }

  const std::string& name() const override { return output_->name(); }

  void onWrite(float voltage) override { output_->write(voltage); }

 private:
  VoltageSink* output_;
  bool owned_;
};

// Adapter from pure input.
class Input : public FakeGpioPin {
 public:
  Input(const VoltageSource* input, bool owned)
      : FakeGpioPin(), input_(input), owned_(owned) {}

  ~Input() {
    if (owned_) delete input_;
  }

  const std::string& name() const override { return input_->name(); }

  float read() const override { return input_->read(); }

  void onWrite(float voltage) override {
    LOG(ERROR) << "Writing to a voltage source " << name()
               << " is no-op and probably a bug";
  }

 private:
  const VoltageSource* input_;
  bool owned_;
};

// Adapter from input/output.
class InputOutput : public FakeGpioPin {
 public:
  InputOutput(VoltageIO* io, bool owned)
      : FakeGpioPin(), io_(io), owned_(owned) {}

  ~InputOutput() {
    if (owned_) delete io_;
  }

  const std::string& name() const override { return io_->name(); }

  float read() const override { return io_->read(); }

  void onWrite(float voltage) override { io_->write(voltage); }

 private:
  VoltageIO* io_;
  bool owned_;
};

}  // namespace

FakeGpioInterface::FakeGpioInterface() {}

void FakeGpioInterface::attach(uint8_t pin, VoltageIO& io) {
  attachInternal(pin, new Output(&io, false));
}

void FakeGpioInterface::attach(uint8_t pin, std::unique_ptr<VoltageIO> io) {
  attachInternal(pin, new Output(io.release(), true));
}

void FakeGpioInterface::attachOutput(uint8_t pin, VoltageSink& output) {
  attachInternal(pin, new Output(&output, false));
}

void FakeGpioInterface::attachOutput(uint8_t pin,
                                     std::unique_ptr<VoltageSink> output) {
  attachInternal(pin, new Output(output.release(), true));
}

void FakeGpioInterface::attachInput(uint8_t pin, const VoltageSource& input) {
  attachInternal(pin, new Input(&input, false));
}

void FakeGpioInterface::attachInput(
    uint8_t pin, std::unique_ptr<const VoltageSource> input) {
  attachInternal(pin, new Input(input.release(), true));
}

void FakeGpioInterface::attachInternal(uint8_t pin, FakeGpioPin* fake) {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& val = pins_[pin];
  if (val != nullptr) {
    LOG(ERROR) << "GPIO conflict: attaching " << fake->name() << " on pin "
               << int(pin) << " which had " << pins_[pin]->name()
               << " already attached";
  }
  val.reset(fake);
}

void FakeGpioInterface::detach(uint8_t pin) { pins_[pin].reset(nullptr); }

FakeGpioPin& FakeGpioInterface::get(uint8_t pin) const {
  if (pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& result = pins_[pin];
  if (result == nullptr) {
    result.reset(new Output(new SimpleVoltageSink(), true));
  }
  return *result;
}
