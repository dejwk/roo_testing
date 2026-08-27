#include "fake_gpio.h"

#include <cmath>
#include <cstdlib>

#include "glog/logging.h"
#include "roo_testing/system/timer.h"

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

  void onWrite(const VoltageSignal& signal) override { output_->write(signal); }

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

  float readAtUptimeMicros(int64_t) const override { return input_->read(); }

  void onWrite(const VoltageSignal&) override {
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

  float readAtUptimeMicros(int64_t) const override { return io_->read(); }

  void onWrite(const VoltageSignal& signal) override { io_->write(signal); }

 private:
  VoltageIO* io_;
  bool owned_;
};

}  // namespace

void FakeGpioInterface::attach(int pin, VoltageIO& io) {
  attachInternal(pin, new InputOutput(&io, false));
}

void FakeGpioInterface::attach(int pin, std::unique_ptr<VoltageIO> io) {
  attachInternal(pin, new InputOutput(io.release(), true));
}

void FakeGpioInterface::attachOutput(int pin, VoltageSink& output) {
  attachInternal(pin, new Output(&output, false));
}

void FakeGpioInterface::attachOutput(int pin,
                                     std::unique_ptr<VoltageSink> output) {
  attachInternal(pin, new Output(output.release(), true));
}

void FakeGpioInterface::attachInput(int pin, const VoltageSource& input) {
  attachInternal(pin, new Input(&input, false));
}

void FakeGpioInterface::attachInput(
    int pin, std::unique_ptr<const VoltageSource> input) {
  attachInternal(pin, new Input(input.release(), true));
}

void FakeGpioInterface::attachInternal(int pin, FakeGpioPin* fake) {
  CHECK_GE(pin, 0);
  CHECK_LT(pin, size_);
  if ((size_t)pin >= pins_.size()) {
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

void FakeGpioInterface::detach(int pin) { pins_[pin].reset(nullptr); }

FakeGpioPin& FakeGpioInterface::get(int pin) const {
  if ((size_t)pin >= pins_.size()) {
    pins_.resize(pin + 1);
  }
  auto& result = pins_[pin];
  if (result == nullptr) {
    result.reset(new Output(new SimpleVoltageSink(), true));
  }
  return *result;
}

float FakeGpioPin::read() const {
  return readAtUptimeMicros(system_time_get_micros());
}

float FakeGpioPin::readAtUptimeMicros(int64_t uptime_us) const {
  const std::optional<VoltageSignal> signal = lastSignal();
  if (!signal.has_value()) {
    // An unwritten output pin remains floating, as before signal support.
    return static_cast<float>(rand()) * 5.0f / RAND_MAX;
  }
  return signal->voltageAtUptimeMicros(uptime_us);
}

DigitalLevel FakeGpioPin::digitalRead() const {
  return DigitalLevelFromVoltage(read());
}

bool FakeGpioPin::isDigitalLow() const { return digitalRead() == kDigitalLow; }

bool FakeGpioPin::isDigitalHigh() const {
  return digitalRead() == kDigitalHigh;
}

void FakeGpioPin::write(const VoltageSignal& signal) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    last_signal_ = signal;
  }
  onWrite(signal);
}

void FakeGpioPin::write(float voltage) {
  write(VoltageSignal::Constant(voltage));
}

void FakeGpioPin::digitalWrite(DigitalLevel level) {
  write(VoltageFromDigitalLevel(level));
}

void FakeGpioPin::digitalWriteHigh() { digitalWrite(kDigitalHigh); }

void FakeGpioPin::digitalWriteLow() { digitalWrite(kDigitalLow); }

std::optional<VoltageSignal> FakeGpioPin::lastSignal() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_signal_;
}

float FakeGpioPin::last_written() const {
  const std::optional<VoltageSignal> signal = lastSignal();
  return signal.has_value() ? AverageDcVoltage(*signal) : std::nanf("");
}
