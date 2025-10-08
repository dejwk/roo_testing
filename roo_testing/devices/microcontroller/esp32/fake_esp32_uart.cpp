#include "fake_esp32_uart.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>

#include "fake_esp32.h"
#include "glog/logging.h"

Esp32UartInterface::Esp32UartInterface(int idx, const std::string& name,
                                       uint8_t tx_signal, uint8_t rx_signal,
                                       FakeEsp32Board* esp32)
    : idx_(idx), tx_signal_(tx_signal), rx_signal_(rx_signal), esp32_(esp32) {}

size_t Esp32UartInterface::write(const uint8_t* buf, size_t size) {
  // Find the device to communicate with.
  for (const auto& i : esp32_->uart_devices()) {
    if (esp32_->out_matrix.signal_for_pin(i.second.tx) != tx_signal_) continue;
    FakeUartDevice* dev = i.first;
    return dev->write(buf, size);
  }
  return 0;
}

size_t Esp32UartInterface::availableForWrite() {
  // Find the device to communicate with.
  for (const auto& i : esp32_->uart_devices()) {
    if (esp32_->out_matrix.signal_for_pin(i.second.tx) != tx_signal_) continue;
    FakeUartDevice* dev = i.first;
    return dev->availableForWrite();
  }
  return 0;
}

size_t Esp32UartInterface::read(uint8_t* buf, size_t size) {
  // Find the device to communicate with.
  for (const auto& i : esp32_->uart_devices()) {
    if (esp32_->in_matrix.pin_for_signal(rx_signal_) != i.second.rx) continue;
    FakeUartDevice* dev = i.first;
    return dev->read(buf, size);
  }
  return 0;
}

size_t Esp32UartInterface::availableForRead() {
  // Find the device to communicate with.
  for (const auto& i : esp32_->uart_devices()) {
    if (esp32_->in_matrix.pin_for_signal(rx_signal_) != i.second.rx) continue;
    FakeUartDevice* dev = i.first;
    return dev->availableForRead();
  }
  return 0;
}

extern "C" {
void uart_notify_data_available(uint8_t uart_num);
}

void Esp32UartInterface::notifyDataAvailable() {
  uart_notify_data_available(idx_);
}