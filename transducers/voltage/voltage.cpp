#include "roo_testing/transducers/voltage/voltage.h"

#include "glog/logging.h"

namespace roo_testing_transducers {

const ConstVoltage& Vcc33() {
  static ConstVoltage vcc33("VCC+3.3V", 3.3);
  return vcc33;
}

const ConstVoltage& Ground() {
  static ConstVoltage ground("Ground", 0);
  return ground;
}

void SimpleVoltageSink::warnIfUnwrittenTo() const {
  if (!has_been_written_) {
    LOG_EVERY_N(WARNING, 10000)
        << name()
        << " has never been written to; it is likely unconnected, or "
           "configured as an input.";
  }
}

void SimpleDigitalSink::warnIfUndef() const {
  if (!has_been_written_) {
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
