#pragma once

#include <cmath>
#include <functional>
#include <string>

#include "roo_testing/transducers/transducer.h"

namespace roo_testing_transducers {

enum DigitalLevel {
  kDigitalLow = 0,
  kDigitalHigh = 1,
  kDigitalUndef = -1,
};

static constexpr float VoltageDigitalLow() { return 0.0; }
static constexpr float VoltageDigitalHigh() { return 3.3; }

static constexpr DigitalLevel DigitalLevelFromVoltage(float voltage) {
  return voltage <= 0.8   ? kDigitalLow
         : voltage >= 2.0 ? kDigitalHigh
                          : kDigitalUndef;
}

static constexpr float VoltageFromDigitalLevel(DigitalLevel level) {
  return (level == kDigitalHigh ? 3.3 : level == kDigitalLow ? 0.0 : 1.5);
}

class VoltageSource : public Transducer {
 public:
  VoltageSource() {}

  virtual float read() const = 0;
};

class VoltageSink : public Transducer {
 public:
  VoltageSink() {}

  virtual void write(float voltage) = 0;
};

class SimpleVoltageSource : public VoltageSource {
 public:
  SimpleVoltageSource(std::function<float()> voltage_fn)
      : SimpleVoltageSource("<unnamed>", voltage_fn) {}

  SimpleVoltageSource(std::string name, std::function<float()> voltage_fn)
      : VoltageSource(),
        name_(std::move(name)),
        voltage_fn_(std::move(voltage_fn)) {}

  const std::string& name() const override { return name_; }

  float read() const override { return voltage_fn_(); }

 private:
  std::string name_;
  std::function<float()> voltage_fn_;
};

class SimpleDigitalSource : public VoltageSource {
 public:
  SimpleDigitalSource(std::function<DigitalLevel()> level_fn)
      : SimpleDigitalSource("<unnamed>", level_fn) {}

  SimpleDigitalSource(std::string name, std::function<DigitalLevel()> level_fn)
      : VoltageSource(),
        name_(std::move(name)),
        level_fn_(std::move(level_fn)) {}

  const std::string& name() const override { return name_; }

  float read() const override { return VoltageFromDigitalLevel(level_fn_()); }

 private:
  std::string name_;
  std::function<DigitalLevel()> level_fn_;
};

class ConstVoltage : public VoltageSource {
 public:
  ConstVoltage(float value) : ConstVoltage("<unnamed>", value) {}

  ConstVoltage(std::string name, float value)
      : VoltageSource(), name_(std::move(name)), value_(std::move(value)) {}

  const std::string& name() const override { return name_; }

  float read() const override { return value_; }

  void set(float value) { value_ = value; }

  void set(DigitalLevel level) { value_ = VoltageFromDigitalLevel(level); }

  void setDigitalLow() { value_ = 0.0; }

 private:
  std::string name_;
  float value_;
};

class SimpleVoltageSink : public VoltageSink {
 public:
  SimpleVoltageSink() : SimpleVoltageSink("<unnamed>", nullptr) {}

  SimpleVoltageSink(std::function<void(float)> write)
      : SimpleVoltageSink("<unnamed>", write) {}

  SimpleVoltageSink(std::string name) : SimpleVoltageSink(name, nullptr) {}

  SimpleVoltageSink(std::string name, std::function<void(float)> write)
      : VoltageSink(),
        name_(std::move(name)),
        write_fn_(std::move(write)),
        last_written_(std::nanf("")) {}

  const std::string& name() const override { return name_; }

  void write(float voltage) override {
    if (write_fn_ != nullptr) write_fn_(voltage);
    last_written_ = voltage;
    has_been_written_ = true;
  }

  float voltage() const { return last_written_; }

  DigitalLevel digitalValue() const {
    return DigitalLevelFromVoltage(voltage());
  }

  bool isDigitalLow() const { return digitalValue() == kDigitalLow; }

  bool isDigitalHigh() const { return digitalValue() == kDigitalHigh; }

  void warnIfUnwrittenTo() const;

 private:
  std::string name_;
  std::function<void(float)> write_fn_;

  float last_written_;
  bool has_been_written_;
};

class SimpleDigitalSink : public VoltageSink {
 public:
  SimpleDigitalSink() : SimpleDigitalSink("<unnamed>", nullptr) {}

  SimpleDigitalSink(std::function<void(DigitalLevel)> write)
      : SimpleDigitalSink("<unnamed>", write) {}

  SimpleDigitalSink(std::string name) : SimpleDigitalSink(name, nullptr) {}

  SimpleDigitalSink(std::string name, std::function<void(DigitalLevel)> write)
      : VoltageSink(),
        name_(std::move(name)),
        write_fn_(std::move(write)),
        last_written_(kDigitalUndef),
        has_been_written_(false) {}

  const std::string& name() const override { return name_; }

  void write(float voltage) override {
    auto val = DigitalLevelFromVoltage(voltage);
    if (write_fn_ != nullptr) write_fn_(val);
    last_written_ = val;
    has_been_written_ = true;
  }

  DigitalLevel value() const { return last_written_; }

  bool isLow() const { return value() == kDigitalLow; }
  bool isHigh() const { return value() == kDigitalHigh; }

  void warnIfUndef() const;

 private:
  std::string name_;
  std::function<void(DigitalLevel)> write_fn_;

  DigitalLevel last_written_;
  bool has_been_written_;
};

class VoltageIO : public VoltageSource, public VoltageSink {
 public:
  VoltageIO() {}

  const std::string& name() const override { return VoltageSource::name(); }
};

const ConstVoltage& Vcc33();
const ConstVoltage& Ground();

}  // namespace roo_testing_transducers
