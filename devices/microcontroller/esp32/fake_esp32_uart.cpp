#include "fake_esp32_uart.h"

#include "fake_esp32.h"

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <set>
#include <thread>

#include "glog/logging.h"

// volatile SpiDevType SPI0;                                      /* SPI0 IS FOR INTERNAL USE*/
// volatile SpiDevType SPI1;
// volatile SpiDevType SPI2;
// volatile SpiDevType SPI3;

Esp32UartInterface::Esp32UartInterface(const std::string& name,
                                       uint8_t tx_signal, uint8_t rx_signal,
                                       FakeEsp32Board* esp32)
    : FakeUartInterface(name),
      tx_signal_(tx_signal),
      rx_signal_(rx_signal),
      esp32_(esp32) {}

void Esp32UartInterface::write(const uint8_t* buf, size_t size) {
  // Find the device to communicate with.
  for (const auto& i : esp32_->uart_devices()) {
    if (esp32_->out_matrix.signal_for_pin(i.second.tx) != tx_signal_)
      continue;
    FakeUartDevice* dev = i.first;
    dev->write(buf, size);
  }
}
