#include "fake_i2c.h"

#include "soc/soc_caps.h"

FakeI2cInterface::~FakeI2cInterface() {
  for (auto& i : devices_) {
    if (i.second.owned) {
      delete i.second.ptr;
    }
  }
}

void FakeI2cInterface::attachInternal(FakeI2cDevice* device, bool owned) {
  uint16_t address = device->address();
  devices_.insert(std::make_pair(address, Attachment(device, owned)));
}

FakeI2cDevice* FakeI2cInterface::getDevice(uint16_t address) const {
  auto i = devices_.find(address);
  return (i == devices_.end()) ? nullptr : i->second.ptr;
}

FakeI2cInterface* getI2cInterface(uint8_t i2c_num) {
  // Keep the selected chip's controller objects alive
  // for the process lifetime so legacy devices can attach once and retrieve
  // the same emulated bus on every subsequent access.
  static FakeI2cInterface i2c0("i2c0");
#if SOC_I2C_NUM > 1
  static FakeI2cInterface i2c1("i2c1");
#endif
  switch (i2c_num) {
    case 0:
      return &i2c0;
#if SOC_I2C_NUM > 1
    case 1:
      return &i2c1;
#endif
    default:
      return nullptr;
  }
}

static FakeI2cDevice* getDevice(uint8_t i2c_num, uint16_t address) {
  FakeI2cInterface* i2c = getI2cInterface(i2c_num);
  return (i2c == nullptr) ? nullptr : i2c->getDevice(address);
}

extern "C" {

uint8_t i2cFakeWrite(uint8_t i2c_num, uint16_t address, uint8_t* buff,
                     uint16_t size, bool sendStop, uint16_t timeOutMillis) {
  FakeI2cDevice* device = getDevice(i2c_num, address);
  if (device == nullptr) {
    return FakeI2cDevice::I2C_ERROR_DEV;
  } else {
    return device->write(buff, size, sendStop, timeOutMillis);
  }
}

uint8_t i2cFakeRead(uint8_t i2c_num, uint16_t address, uint8_t* buff,
                    uint16_t size, bool sendStop, uint16_t timeOutMillis) {
  FakeI2cDevice* device = getDevice(i2c_num, address);
  if (device == nullptr) {
    return FakeI2cDevice::I2C_ERROR_DEV;
  } else {
    return device->read(buff, size, sendStop, timeOutMillis);
  }
}
}
