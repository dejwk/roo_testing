#include "SPI.h"

#include <algorithm>
#include <array>

#include "glog/logging.h"

namespace {
/// Validates the RP2040 alternate routes for a specific SPI signal.
bool ValidPin(spi_inst_t* spi, pin_size_t pin, uint role) {
  return pin == 255 ||
         (pin < 30 && pin % 4 == role && (pin / 8) % 2 == spi_get_index(spi));
}
}  // namespace

SPIClassRP2040 SPI(spi0, PIN_SPI0_MISO, PIN_SPI0_SS, PIN_SPI0_SCK,
                   PIN_SPI0_MOSI);
SPIClassRP2040 SPI1(spi1, PIN_SPI1_MISO, PIN_SPI1_SS, PIN_SPI1_SCK,
                    PIN_SPI1_MOSI);

SPIClassRP2040::SPIClassRP2040(spi_inst_t* spi, pin_size_t rx, pin_size_t cs,
                               pin_size_t sck, pin_size_t tx)
    : _spi(spi),
      _RX(rx),
      _TX(tx),
      _SCK(sck),
      _CS(cs),
      _hwCS(false),
      _running(false),
      _initted(false) {}

bool SPIClassRP2040::setRX(pin_size_t pin) {
  if (_running || !ValidPin(_spi, pin, 0)) return false;
  _RX = pin;
  return true;
}

bool SPIClassRP2040::setTX(pin_size_t pin) {
  if (_running || !ValidPin(_spi, pin, 3)) return false;
  _TX = pin;
  return true;
}

bool SPIClassRP2040::setSCK(pin_size_t pin) {
  if (_running || pin == 255 || !ValidPin(_spi, pin, 2)) return false;
  _SCK = pin;
  return true;
}

bool SPIClassRP2040::setCS(pin_size_t pin) {
  if (_running || pin >= 30) return false;
  _CS = pin;
  return true;
}

void SPIClassRP2040::begin(bool hw_cs) {
  CHECK(!hw_cs) << "Unimplemented: Arduino-Pico hardware chip select";
  if (_running) return;
  spi_init(_spi, 1000000);
  for (pin_size_t pin : {_RX, _TX, _SCK})
    if (pin != 255) gpio_set_function(pin, GPIO_FUNC_SPI);
  _running = true;
}

void SPIClassRP2040::end() {
  if (!_running) return;
  spi_deinit(_spi);
  for (pin_size_t pin : {_RX, _TX, _SCK})
    if (pin != 255) gpio_set_function(pin, GPIO_FUNC_NULL);
  _running = false;
  _initted = false;
}

void SPIClassRP2040::beginTransaction(SPISettings settings) {
  CHECK(_running) << "SPI.begin() required";
  _spis = settings;
  spi_set_baudrate(_spi, settings.getClockFreq());
  spi_set_format(_spi, 8, _helper.cpol(settings), _helper.cpha(settings),
                 SPI_MSB_FIRST);
  _helper.maskInterrupts();
  _initted = true;
}

void SPIClassRP2040::endTransaction() { _helper.unmaskInterrupts(); }

void SPIClassRP2040::transfer(const void* txbuf, void* rxbuf, size_t count) {
  CHECK(_running);
  if (!_initted) beginTransaction(SPISettings());
  const uint8_t* tx = static_cast<const uint8_t*>(txbuf);
  uint8_t* rx = static_cast<uint8_t*>(rxbuf);
  // Bound scratch storage and retain one SDK transfer per chunk, not per byte.
  std::array<uint8_t, 256> outgoing;
  std::array<uint8_t, 256> incoming;
  for (size_t offset = 0; offset < count;) {
    const size_t size = std::min(count - offset, outgoing.size());
    for (size_t i = 0; i < size; ++i) {
      const uint8_t byte = tx == nullptr ? 0 : tx[offset + i];
      outgoing[i] =
          _spis.getBitOrder() == LSBFIRST ? _helper.reverseByte(byte) : byte;
    }
    CHECK_EQ(
        spi_write_read_blocking(_spi, outgoing.data(), incoming.data(), size),
        static_cast<int>(size));
    if (rx != nullptr)
      for (size_t i = 0; i < size; ++i)
        rx[offset + i] = _spis.getBitOrder() == LSBFIRST
                             ? _helper.reverseByte(incoming[i])
                             : incoming[i];
    offset += size;
  }
}

void SPIClassRP2040::transfer(void* bytes, size_t size) {
  transfer(bytes, bytes, size);
}

uint8_t SPIClassRP2040::transfer(uint8_t byte) {
  uint8_t result;
  transfer(&byte, &result, 1);
  return result;
}

uint16_t SPIClassRP2040::transfer16(uint16_t word) {
  uint8_t bytes[2];
  bytes[0] = _spis.getBitOrder() == LSBFIRST ? word : word >> 8;
  bytes[1] = _spis.getBitOrder() == LSBFIRST ? word >> 8 : word;
  transfer(bytes, 2);
  return _spis.getBitOrder() == LSBFIRST
             ? uint16_t(bytes[0]) | (uint16_t(bytes[1]) << 8)
             : (uint16_t(bytes[0]) << 8) | bytes[1];
}

bool SPIClassRP2040::transferAsync(const void*, void*, size_t) {
  LOG(WARNING) << "Unimplemented: Pico SPI DMA";
  return false;
}

bool SPIClassRP2040::finishedAsync() {
  LOG(FATAL) << "Unimplemented: Pico SPI DMA";
  return false;
}

void SPIClassRP2040::abortAsync() {
  LOG(FATAL) << "Unimplemented: Pico SPI DMA";
}

void SPIClassRP2040::onTransferComplete(void (*)(void)) {
  LOG(FATAL) << "Unimplemented: Pico SPI DMA callback";
}

void SPIClassRP2040::onTransferComplete(void (*)(void*), void*) {
  LOG(FATAL) << "Unimplemented: Pico SPI DMA callback";
}

void SPIClassRP2040::setBitOrder(BitOrder order) {
  beginTransaction(
      SPISettings(_spis.getClockFreq(), order, _spis.getDataMode()));
}

void SPIClassRP2040::setDataMode(uint8_t mode) {
  beginTransaction(
      SPISettings(_spis.getClockFreq(), _spis.getBitOrder(), mode));
}

void SPIClassRP2040::setClockDivider(uint8_t divider) {
  CHECK_NE(divider, 0);
  beginTransaction(
      SPISettings(F_CPU / divider, _spis.getBitOrder(), _spis.getDataMode()));
}
