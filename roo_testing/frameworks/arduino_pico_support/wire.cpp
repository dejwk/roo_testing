#include "Wire.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "glog/logging.h"
#include "pico/error.h"
#include "roo_testing/frameworks/pico_support/task_support.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

TwoWire Wire(i2c0, PIN_WIRE0_SDA, PIN_WIRE0_SCL);
TwoWire Wire1(i2c1, PIN_WIRE1_SDA, PIN_WIRE1_SCL);

TwoWire::TwoWire(i2c_inst_t* i2c, pin_size_t sda, pin_size_t scl)
    : _i2c(i2c),
      _sda(sda),
      _scl(scl),
      _clkHz(TWI_CLOCK),
      _running(false),
      _slave(false),
      _addr(0),
      _txBegun(false),
      _timeoutFlag(false),
      _reset_with_timeout(false),
      _buff(nullptr),
      _buffSize(0),
      _buffLen(0),
      _buffOff(0),
      _onRequestCallback(nullptr),
      _onReceiveCallback(nullptr) {
  CHECK_EQ(setBufferSize(WIRE_BUFFER_SIZE), size_t{WIRE_BUFFER_SIZE});
  _timeout = 25;
}

size_t TwoWire::setBufferSize(size_t size) {
  if (_running || size < WIRE_BUFFER_SIZE_MIN || size > 65535) return 0;
  void* buffer = std::realloc(_buff, size);
  if (buffer == nullptr) return 0;
  _buff = static_cast<uint8_t*>(buffer);
  _buffSize = size;
  _buffLen = 0;
  _buffOff = 0;
  return size;
}

bool TwoWire::setSDA(pin_size_t pin) {
  if (_running || pin >= 30 || pin % 2 != 0 ||
      (pin / 2) % 2 != i2c_get_index(_i2c))
    return false;
  _sda = pin;
  return true;
}

bool TwoWire::setSCL(pin_size_t pin) {
  if (_running || pin >= 30 || pin % 2 != 1 ||
      (pin / 2) % 2 != i2c_get_index(_i2c))
    return false;
  _scl = pin;
  return true;
}

void TwoWire::begin() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  if (_running) end();
  i2c_init(_i2c, _clkHz);
  gpio_set_function(_sda, GPIO_FUNC_I2C);
  gpio_set_function(_scl, GPIO_FUNC_I2C);
  _running = true;
  _txBegun = false;
  _buffLen = 0;
  _buffOff = 0;
}

void TwoWire::begin(uint8_t) { LOG(FATAL) << "Unimplemented: Pico I2C slave"; }
void TwoWire::end() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  if (!_running) return;
  i2c_deinit(_i2c);
  gpio_set_function(_sda, GPIO_FUNC_NULL);
  gpio_set_function(_scl, GPIO_FUNC_NULL);
  _running = false;
  _txBegun = false;
  _buffLen = 0;
  _buffOff = 0;
}

void TwoWire::setClock(uint32_t frequency) {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  CHECK_GT(frequency, 0u);
  _clkHz = frequency;
  if (_running) i2c_set_baudrate(_i2c, frequency);
}

void TwoWire::beginTransmission(uint8_t address) {
  roo_testing::pico::ReserveController(2 + i2c_get_index(_i2c));
  CHECK(_running);
  _addr = address;
  _buffLen = 0;
  _buffOff = 0;
  _txBegun = true;
}

size_t TwoWire::write(uint8_t byte) { return write(&byte, 1); }
size_t TwoWire::write(const uint8_t* bytes, size_t size) {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  if (!_txBegun) return 0;
  const size_t written = std::min(size, _buffSize - _buffLen);
  if (written != 0) std::memcpy(_buff + _buffLen, bytes, written);
  _buffLen += written;
  if (written < size) setWriteError();
  return written;
}

uint8_t TwoWire::endTransmission(bool stop) {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  if (!_running || !_txBegun) return 4;
  _txBegun = false;
  const int result = i2c_write_timeout_us(
      _i2c, _addr, _buff, _buffLen, !stop,
      std::min<uint64_t>(uint64_t{_timeout} * 1000, UINT32_MAX));
  if (result == PICO_ERROR_TIMEOUT) {
    _timeoutFlag = true;
    if (_reset_with_timeout) {
      end();
      begin();
    }
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
    return 5;
  }
  if (stop || result < 0)
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  return result < 0 ? 4 : 0;
}

uint8_t TwoWire::endTransmission() { return endTransmission(true); }
size_t TwoWire::requestFrom(uint8_t address, size_t size, bool stop) {
  roo_testing::pico::ReserveController(2 + i2c_get_index(_i2c));
  if (!_running || _txBegun) {
    if (!_txBegun)
      roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
    return 0;
  }
  size = std::min(size, _buffSize);
  _buffLen = 0;
  _buffOff = 0;
  if (size == 0) {
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
    return 0;
  }
  const int result = i2c_read_timeout_us(
      _i2c, address, _buff, size, !stop,
      std::min<uint64_t>(uint64_t{_timeout} * 1000, UINT32_MAX));
  if (result == PICO_ERROR_TIMEOUT) {
    _timeoutFlag = true;
    if (_reset_with_timeout) {
      end();
      begin();
    }
  }
  if (result > 0) _buffLen = result;
  if (_buffLen == 0)
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  return _buffLen;
}

size_t TwoWire::requestFrom(uint8_t address, size_t size) {
  return requestFrom(address, size, true);
}

int TwoWire::available() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  const int count = _buffLen - _buffOff;
  if (!_txBegun && _buffOff >= _buffLen &&
      !FakePico().i2cState(i2c_get_index(_i2c)).restart_on_next)
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  return count;
}
int TwoWire::read() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  const int value = _buffOff < _buffLen ? _buff[_buffOff++] : -1;
  if (!_txBegun && _buffOff >= _buffLen &&
      !FakePico().i2cState(i2c_get_index(_i2c)).restart_on_next)
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  return value;
}
int TwoWire::peek() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  return _buffOff < _buffLen ? _buff[_buffOff] : -1;
}
void TwoWire::flush() {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  if (!FakePico().i2cState(i2c_get_index(_i2c)).restart_on_next) {
    roo_testing::pico::ReleaseController(2 + i2c_get_index(_i2c));
  }
  _buffLen = 0;
  _buffOff = 0;
}

void TwoWire::setTimeout(uint32_t timeout, bool reset) {
  roo_testing::pico::ControllerLock operation(2 + i2c_get_index(_i2c));
  _timeout = timeout;
  _reset_with_timeout = reset;
}

bool TwoWire::getTimeoutFlag() { return _timeoutFlag; }
void TwoWire::clearTimeoutFlag() { _timeoutFlag = false; }
void TwoWire::onReceive(void (*)(int)) {
  LOG(FATAL) << "Unimplemented: Pico I2C slave callbacks";
}

void TwoWire::onRequest(void (*)()) {
  LOG(FATAL) << "Unimplemented: Pico I2C slave callbacks";
}

bool TwoWire::writeReadAsync(uint8_t, const void*, size_t, const void*, size_t,
                             bool) {
  LOG(WARNING) << "Unimplemented: Pico I2C DMA";
  return false;
}

bool TwoWire::writeAsync(uint8_t, const void*, size_t, bool) {
  LOG(WARNING) << "Unimplemented: Pico I2C DMA";
  return false;
}

bool TwoWire::readAsync(uint8_t, void*, size_t, bool) {
  LOG(WARNING) << "Unimplemented: Pico I2C DMA";
  return false;
}

bool TwoWire::finishedAsync() {
  LOG(FATAL) << "Unimplemented: Pico I2C DMA";
  return false;
}

bool TwoWire::busIdle() {
  return true;
}  // Supported calls are synchronous on core 0.
void TwoWire::abortAsync() { LOG(FATAL) << "Unimplemented: Pico I2C DMA"; }
void TwoWire::onFinishedAsync(void (*)()) {
  LOG(FATAL) << "Unimplemented: Pico I2C DMA callbacks";
}

void TwoWire::onIRQ() { LOG(FATAL) << "Unimplemented: Pico I2C interrupts"; }
void TwoWire::_dma_irq_handler() {
  LOG(FATAL) << "Unimplemented: Pico I2C DMA interrupts";
}
