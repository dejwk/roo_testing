#pragma once

#include <functional>
#include <string>

namespace roo_testing_transducers {

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

  void set_value(float value) { value_ = value; }

 private:
  std::string name_;
  float value_;
};

const ConstVoltage& Vcc33();
const ConstVoltage& Ground();

}  // namespace roo_testing_transducers
