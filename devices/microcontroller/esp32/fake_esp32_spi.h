#pragma once

#include "roo_testing/buses/spi/fake_spi.h"

#include "fake_esp32_spi_struct.h"

class FakeEsp32Board;

class Esp32SpiInterface : public FakeSpiInterface {
 public:
  Esp32SpiInterface(volatile SpiDevType& spi, const std::string& name,
                    uint8_t clk_signal, uint8_t miso_signal,
                    uint8_t mosi_signal, FakeEsp32Board* esp32);

  uint32_t clkHz() const override;
  SpiDataMode dataMode() const override;
  SpiBitOrder bitOrder() const override;

 private:
  friend void spiFakeTransferOnDevice(int8_t spi_num);

  // Returns the number of SPI cycles spent.
  uint32_t transfer();

  volatile SpiDevType& spi_;

  uint8_t clk_signal_;
  uint8_t miso_signal_;
  uint8_t mosi_signal_;

  FakeEsp32Board* esp32_;
};
