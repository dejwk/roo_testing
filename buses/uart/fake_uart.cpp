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

FakeUartDevice::Result ConsoleUartDevice::write(const uint8_t* buf, uint16_t size) {
  std::cout.write((const char*)buf, size);
  return FakeUartDevice::UART_ERROR_OK;
}

FakeUartDevice::Result ConsoleUartDevice::read(uint8_t* buf, uint16_t size) {
  std::istream& is = std::cin;
  while (size > 0) {
    int read = is.readsome((char*)buf, size);
    if (read <= 0) {
      while (size-- > 0) *buf++ = 0;
      return FakeUartDevice::UART_ERROR_OK;
    }
    buf += read;
    size -= read;
  }
  return UART_ERROR_OK;
}
