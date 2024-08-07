#include "roo_testing/devices/port_expander/pcf8574/pcf8574.h"

#include <assert.h>

#include <cstring>

#include "glog/logging.h"

using namespace roo_testing_transducers;

FakeI2cPcf8574::FakeI2cPcf8574(uint8_t addr)
    : FakeI2cDevice("PCF8574", 0x20 + addr), gpio(8), initialized_(false) {
  CHECK_LT(addr, 8);
}

FakeI2cDevice::Result FakeI2cPcf8574::write(const uint8_t *buf, uint16_t size,
                                            bool sendStop,
                                            uint16_t timeOutMillis) {
  CHECK_EQ(1, size);
  uint8_t data = buf[0];
  gpio.get(0).digitalWrite(data & 0x01 ? kDigitalHigh : kDigitalLow);
  gpio.get(1).digitalWrite(data & 0x02 ? kDigitalHigh : kDigitalLow);
  gpio.get(2).digitalWrite(data & 0x04 ? kDigitalHigh : kDigitalLow);
  gpio.get(3).digitalWrite(data & 0x08 ? kDigitalHigh : kDigitalLow);
  gpio.get(4).digitalWrite(data & 0x10 ? kDigitalHigh : kDigitalLow);
  gpio.get(5).digitalWrite(data & 0x20 ? kDigitalHigh : kDigitalLow);
  gpio.get(6).digitalWrite(data & 0x40 ? kDigitalHigh : kDigitalLow);
  gpio.get(7).digitalWrite(data & 0x80 ? kDigitalHigh : kDigitalLow);
  return I2C_ERROR_OK;
}

FakeI2cDevice::Result FakeI2cPcf8574::read(uint8_t *buf, uint16_t size,
                                           bool sendStop,
                                           uint16_t timeOutMillis) {
  CHECK_EQ(1, size);
  if (!initialized_) {
    gpio.get(0).digitalWrite(kDigitalHigh);
    gpio.get(1).digitalWrite(kDigitalHigh);
    gpio.get(2).digitalWrite(kDigitalHigh);
    gpio.get(3).digitalWrite(kDigitalHigh);
    gpio.get(4).digitalWrite(kDigitalHigh);
    gpio.get(5).digitalWrite(kDigitalHigh);
    gpio.get(6).digitalWrite(kDigitalHigh);
    gpio.get(7).digitalWrite(kDigitalHigh);
    initialized_ = true;
  }
  buf[0] = (gpio.get(0).digitalRead() << 0) | (gpio.get(1).digitalRead() << 1) |
           (gpio.get(2).digitalRead() << 2) | (gpio.get(3).digitalRead() << 3) |
           (gpio.get(4).digitalRead() << 4) | (gpio.get(5).digitalRead() << 5) |
           (gpio.get(6).digitalRead() << 6) | (gpio.get(7).digitalRead() << 7);
  return I2C_ERROR_OK;
}
