#include "roo_testing/devices/clock/internal/i2c_rtc.h"

namespace roo_testing_rtc {
I2cRtc::I2cRtc(const char* name, uint8_t address, Chip chip,
               roo_testing_transducers::Clock* clock)
    : FakeI2cDevice(name, address), registers_(chip, clock) {}

FakeI2cDevice::Result I2cRtc::write(const uint8_t* data, uint16_t size,
                                    bool stop, uint16_t timeout) {
  if (data == nullptr || size == 0 || data[0] >= registers_.size()) {
    return I2C_ERROR_ACK;
  }
  address_ = data[0];
  for (uint16_t i = 1; i < size; ++i) {
    registers_.write(address_, data[i]);
    address_ = registers_.nextAddress(address_);
  }
  return I2C_ERROR_OK;
}

FakeI2cDevice::Result I2cRtc::read(uint8_t* data, uint16_t size, bool stop,
                                   uint16_t timeout) {
  if (data == nullptr || size == 0) return I2C_ERROR_ACK;
  registers_.beginRead();
  for (uint16_t i = 0; i < size; ++i) {
    data[i] = registers_.read(address_);
    address_ = registers_.nextAddress(address_);
    if (address_ == 0) registers_.beginRead();
  }
  registers_.endRead();
  return I2C_ERROR_OK;
}

void I2cRtc::simulateTimeLoss() { registers_.simulateTimeLoss(); }

void I2cRtc::injectRegister(uint8_t address, uint8_t value) {
  registers_.injectRegister(address, value);
}

void I2cRtc::reset() {
  registers_.reset();
  address_ = 0;
}

}  // namespace roo_testing_rtc
