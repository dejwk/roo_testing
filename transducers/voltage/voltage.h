#pragma once

#include <functional>
#include <string>

namespace roo_testing_transducers {

enum DigitalLevel {
  kDigitalLow = 0, kDigitalHigh = 1, kDigitalUndef = -1,
};

static constexpr DigitalLevel DigitalLevelFromVoltage(float voltage) {
  return voltage <= 0.8   ? kDigitalLow
          : voltage >= 2.0 ? kDigitalHigh
                          : kDigitalUndef;
}

class Voltage {
 public:
  Voltage() {}
  virtual ~Voltage() {}

  virtual std::string name() const { return "<unnamed>"; }

  virtual float read() const = 0;
};

class SimpleVoltage : public Voltage {
 public:
  SimpleVoltage(std::function<float()> voltage)
      : SimpleVoltage("<unnamed>", voltage) {}

  SimpleVoltage(std::string name, std::function<float()> voltage)
      : Voltage(), name_(std::move(name)), voltage_(std::move(voltage)) {}

  std::string name() const { return name_; }

  float read() const override { return voltage_(); }

 private:
  std::string name_;
  std::function<float()> voltage_;
};

class ConstVoltage : public Voltage {
 public:
  ConstVoltage(float value) : ConstVoltage("<unnamed>", value) {}

  ConstVoltage(std::string name, float value)
      : Voltage(), name_(std::move(name)), value_(std::move(value)) {}

  std::string name() const { return name_; }

  float read() const override { return value_; }

  void set(float value) { value_ = value; }

  void set(DigitalLevel value) {
    value_ = (value == kDigitalHigh ? 3.3 : value == kDigitalLow ? 0.0 : 1.5);
  }

  void setDigitalLow() { value_ = 0.0; }

 private:
  std::string name_;
  float value_;
};

const ConstVoltage& Vcc33();
const ConstVoltage& Ground();

}  // namespace roo_testing_transducers
