#include "fake_i2c.h"

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
