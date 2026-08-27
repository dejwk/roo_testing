#include "roo_testing/transducers/voltage/voltage.h"

#include "glog/logging.h"
#include "roo_testing/system/timer.h"

namespace roo_testing_transducers {

SimpleVoltageSink SimpleVoltageSink::WithSignalCallback(
    std::string name, std::function<void(const VoltageSignal&)> callback) {
  return SimpleVoltageSink(std::move(name), std::move(callback),
                           SignalCallbackTag{});
}

void SimpleVoltageSink::write(const VoltageSignal& signal) {
  std::function<void(const VoltageSignal&)> signal_write_fn;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    signal_ = signal;
    signal_write_fn = signal_write_fn_;
  }
  if (signal_write_fn != nullptr) signal_write_fn(signal);
}

std::optional<VoltageSignal> SimpleVoltageSink::signal() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return signal_;
}

float SimpleVoltageSink::sampleAtUptimeMicros(int64_t uptime_us) const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value() ? current->voltageAtUptimeMicros(uptime_us)
                             : std::nanf("");
}

float SimpleVoltageSink::sample() const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value()
             ? current->voltageAtUptimeMicros(system_time_get_micros())
             : std::nanf("");
}

float SimpleVoltageSink::averageDcVoltageAtUptimeMicros(
    int64_t uptime_us) const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value() ? AverageDcVoltage(*current, uptime_us)
                             : std::nanf("");
}

float SimpleVoltageSink::voltage() const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value() ? AverageDcVoltage(*current) : std::nanf("");
}

SimpleDigitalSink SimpleDigitalSink::WithSignalCallback(
    std::string name, std::function<void(const VoltageSignal&)> callback) {
  return SimpleDigitalSink(std::move(name), std::move(callback),
                           SignalCallbackTag{});
}

void SimpleDigitalSink::write(const VoltageSignal& signal) {
  std::function<void(const VoltageSignal&)> signal_write_fn;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    signal_ = signal;
    signal_write_fn = signal_write_fn_;
  }
  if (signal_write_fn != nullptr) signal_write_fn(signal);
}

std::optional<VoltageSignal> SimpleDigitalSink::signal() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return signal_;
}

DigitalLevel SimpleDigitalSink::instantaneousValueAtUptimeMicros(
    int64_t uptime_us) const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value() ? DigitalLevelFromVoltage(
                                   current->voltageAtUptimeMicros(uptime_us))
                             : kDigitalUndef;
}

DigitalLevel SimpleDigitalSink::instantaneousValue() const {
  return instantaneousValueAtUptimeMicros(system_time_get_micros());
}

DigitalLevel SimpleDigitalSink::value() const {
  const std::optional<VoltageSignal> current = signal();
  return current.has_value()
             ? DigitalLevelFromVoltage(AverageDcVoltage(*current))
             : kDigitalUndef;
}

const ConstVoltage& Vcc33() {
  static ConstVoltage vcc33("VCC+3.3V", 3.3);
  return vcc33;
}

const ConstVoltage& Ground() {
  static ConstVoltage ground("Ground", 0);
  return ground;
}

void SimpleVoltageSink::warnIfUnwrittenTo() const {
  if (!signal().has_value()) {
    LOG_EVERY_N(WARNING, 10000)
        << name()
        << " has never been written to; it is likely unconnected, or "
           "configured as an input.";
  }
}

void SimpleDigitalSink::warnIfUndef() const {
  if (!signal().has_value()) {
    LOG_EVERY_N(WARNING, 10000)
        << name()
        << " has never been written to; it is likely unconnected, or "
           "configured as an input.";
  } else if (value() == kDigitalUndef) {
    LOG_EVERY_N(WARNING, 10000)
        << name()
        << " is in the undefined logical state; it is likely an analog signal.";
  }
}

}  // namespace roo_testing_transducers
