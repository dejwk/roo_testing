#include "fake_uart.h"

#include <iostream>

FakeUartInterface::~FakeUartInterface() {
  if (device_.owned) {
    delete device_.ptr;
  }
}

void FakeUartInterface::attachInternal(FakeUartDevice* device, bool owned) {
  device_ = Attachment(device, owned);
}

FakeUartDevice* FakeUartInterface::getDevice() const {
  return device_.ptr;
}

size_t ConsoleUartDevice::write(const uint8_t* buf, uint16_t size) {
  std::cout.write((const char*)buf, size);
  return size;
}

size_t ConsoleUartDevice::read(uint8_t* buf, uint16_t size) {
  std::istream& is = std::cin;
  size_t total = 0;
  while (size > 0) {
    int read = is.readsome((char*)buf, size);
    if (read <= 0) {
      while (size-- > 0) *buf++ = 0;
      return total;
    }
    buf += read;
    size -= read;
    total += read;
  }
  return total;
}

size_t ConsoleUartDevice::availableForRead() {
  return (std::cin.peek() == std::cin.eof()) ? 0 : 1;
}
