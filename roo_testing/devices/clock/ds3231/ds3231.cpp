#include "roo_testing/devices/clock/ds3231/ds3231.h"

using roo_testing_transducers::FixedThermometer;
using roo_testing_transducers::Temperature;

FakeDs3231::FakeDs3231()
    : FakeDs3231(new FixedThermometer(Temperature::FromC(25))) {}

FakeDs3231::FakeDs3231(roo_testing_transducers::Clock& clock)
    : FakeDs3231(new FixedThermometer(Temperature::FromC(25)), &clock) {}

FakeDs3231::FakeDs3231(roo_testing_transducers::Thermometer* thermometer,
                       roo_testing_transducers::Clock* clock)
    : I2cRtc("DS3231", 0x68, roo_testing_rtc::Chip::kDs3231, clock),
      thermometer_(thermometer) {}

FakeI2cDevice::Result FakeDs3231::write(const uint8_t* data, uint16_t size,
                                        bool stop, uint16_t timeout) {
  registers_.setTemperature(thermometer_->read().AsC());
  return I2cRtc::write(data, size, stop, timeout);
}

FakeI2cDevice::Result FakeDs3231::read(uint8_t* data, uint16_t size, bool stop,
                                       uint16_t timeout) {
  registers_.setTemperature(thermometer_->read().AsC());
  return I2cRtc::read(data, size, stop, timeout);
}
