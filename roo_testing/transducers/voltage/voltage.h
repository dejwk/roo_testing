#pragma once

#include <cmath>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "roo_testing/transducers/transducer.h"
#include "roo_testing/transducers/voltage/voltage_signal.h"

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

  /// Writes a constant voltage signal.
  void write(float voltage) { write(VoltageSignal::Constant(voltage)); }

  /// Writes a complete voltage signal.
  virtual void write(const VoltageSignal& signal) = 0;
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
  /// Creates an unnamed sink with no assignment callback.
  SimpleVoltageSink() : SimpleVoltageSink("<unnamed>") {}

  /// Creates a named sink with no assignment callback.
  SimpleVoltageSink(std::string name)
      : VoltageSink(), name_(std::move(name)), signal_(std::nullopt) {}

  SimpleVoltageSink(const SimpleVoltageSink&) = delete;
  SimpleVoltageSink& operator=(const SimpleVoltageSink&) = delete;
  SimpleVoltageSink(SimpleVoltageSink&&) = delete;
  SimpleVoltageSink& operator=(SimpleVoltageSink&&) = delete;

  const std::string& name() const override { return name_; }

  using VoltageSink::write;

  /// Retains and notifies one complete voltage signal.
  void write(const VoltageSignal& signal) override;

  /// Returns the last assigned signal, if any.
  std::optional<VoltageSignal> signal() const;

  /// Returns an instantaneous sample at an explicit uptime.
  float sampleAtUptimeMicros(int64_t uptime_us) const;

  /// Returns an instantaneous sample at current emulator uptime.
  float sample() const;

  /// Returns local DC voltage at an explicit uptime.
  float averageDcVoltageAtUptimeMicros(int64_t uptime_us) const;

  float voltage() const;

  DigitalLevel digitalValue() const {
    return DigitalLevelFromVoltage(voltage());
  }

  bool isDigitalLow() const { return digitalValue() == kDigitalLow; }

  bool isDigitalHigh() const { return digitalValue() == kDigitalHigh; }

  void warnIfUnwrittenTo() const;

  /// Creates a sink with a waveform-aware assignment callback.
  static SimpleVoltageSink WithSignalCallback(
      std::string name, std::function<void(const VoltageSignal&)> callback);

 private:
  struct SignalCallbackTag {};

  SimpleVoltageSink(std::string name,
                    std::function<void(const VoltageSignal&)> callback,
                    SignalCallbackTag)
      : VoltageSink(),
        name_(std::move(name)),
        signal_write_fn_(std::move(callback)),
        signal_(std::nullopt) {}

  std::string name_;
  std::function<void(const VoltageSignal&)> signal_write_fn_;
  mutable std::mutex mutex_;
  std::optional<VoltageSignal> signal_;
};

class SimpleDigitalSink : public VoltageSink {
 public:
  /// Creates an unnamed sink with no assignment callback.
  SimpleDigitalSink() : SimpleDigitalSink("<unnamed>") {}

  /// Creates a named sink with no assignment callback.
  SimpleDigitalSink(std::string name)
      : VoltageSink(), name_(std::move(name)), signal_(std::nullopt) {}

  SimpleDigitalSink(const SimpleDigitalSink&) = delete;
  SimpleDigitalSink& operator=(const SimpleDigitalSink&) = delete;
  SimpleDigitalSink(SimpleDigitalSink&&) = delete;
  SimpleDigitalSink& operator=(SimpleDigitalSink&&) = delete;

  const std::string& name() const override { return name_; }

  using VoltageSink::write;

  /// Retains and notifies one complete voltage signal.
  void write(const VoltageSignal& signal) override;

  /// Returns the last assigned signal, if any.
  std::optional<VoltageSignal> signal() const;

  /// Returns the digital classification of an explicit instantaneous sample.
  DigitalLevel instantaneousValueAtUptimeMicros(int64_t uptime_us) const;

  /// Returns the digital classification of the current instantaneous sample.
  DigitalLevel instantaneousValue() const;

  DigitalLevel value() const;

  bool isLow() const { return value() == kDigitalLow; }
  bool isHigh() const { return value() == kDigitalHigh; }

  void warnIfUndef() const;

  /// Creates a sink with a waveform-aware assignment callback.
  static SimpleDigitalSink WithSignalCallback(
      std::string name, std::function<void(const VoltageSignal&)> callback);

 private:
  struct SignalCallbackTag {};

  SimpleDigitalSink(std::string name,
                    std::function<void(const VoltageSignal&)> callback,
                    SignalCallbackTag)
      : VoltageSink(),
        name_(std::move(name)),
        signal_write_fn_(std::move(callback)),
        signal_(std::nullopt) {}

  std::string name_;
  std::function<void(const VoltageSignal&)> signal_write_fn_;
  mutable std::mutex mutex_;
  std::optional<VoltageSignal> signal_;
};

class VoltageIO : public VoltageSource, public VoltageSink {
 public:
  VoltageIO() {}

  const std::string& name() const override { return VoltageSource::name(); }
};

const ConstVoltage& Vcc33();
const ConstVoltage& Ground();

}  // namespace roo_testing_transducers
