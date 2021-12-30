#include "fake_spi.h"

#include <glog/logging.h>

#include <iostream>
#include <map>

// FakeSpiInterface::FakeSpiInterface(
//     std::initializer_list<SimpleFakeSpiDevice*> devices) {
//   for (auto device : devices) {
//     attach(device);
//   }
// }

void FakeSpiInterface::attach(SimpleFakeSpiDevice& device) {
  devices_.push_back(&device);
}

extern "C" {

void spiFakeTransfer(uint8_t spi_num, uint32_t clk, SpiDataMode mode,
                     SpiBitOrder order, uint8_t* buf, uint16_t bit_count) {
  FakeSpiInterface* spi = getSpiInterface(spi_num);
  CHECK_NOTNULL(spi);
  SimpleFakeSpiDevice* selected_dev = nullptr;
  for (int i = 0; i < spi->device_count(); i++) {
    SimpleFakeSpiDevice& dev = spi->device(i);
    if (dev.isSelected()) {
      if (selected_dev == nullptr) {
        selected_dev = &dev;
      } else {
        LOG(ERROR) << "SPI interface #" << spi_num << "(" << spi->name()
                   << "): bus conflict: selected " << selected_dev->name()
                   << " and " << dev.name() << "\n";
        return;
      }
    }
  }
  if (selected_dev == nullptr) {
    LOG(WARNING) << "SPI interface #" << spi_num << "(" << spi->name()
                 << "): no device selected";
    return;
  }
  selected_dev->transfer(clk, mode, order, buf, bit_count);
}
}
