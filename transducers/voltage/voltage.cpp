#include "roo_testing/transducers/voltage/voltage.h"

namespace roo_testing_transducers {

const ConstVoltage& Vcc33() {
  static ConstVoltage vcc33("VCC+3.3V", 3.3);
  return vcc33;
}

const ConstVoltage& Ground() {
  static ConstVoltage ground("Ground", 0);
  return ground;
}

}  // namespace roo_testing_transducers
