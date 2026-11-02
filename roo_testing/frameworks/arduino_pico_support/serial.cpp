#include <algorithm>

#include "Arduino.h"
#include "CoreMutex.h"
#include "glog/logging.h"
#include "hardware/uart.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/host/scheduler_signal_guard.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {
/// Validates physical UART routes using the board's exact RP2040 map.
bool ValidPin(uart_inst_t* uart, pin_size_t pin, bool tx) {
  if (pin == UART_PIN_NOT_DEFINED) return true;
  if (pin >= 30 || pin % 4 != (tx ? 0 : 1)) return false;
  constexpr uint8_t controllers[] = {0, 1, 1, 0, 0, 1, 1, 0};
  return controllers[pin / 4] == uart_get_index(uart);
}
}  // namespace

SerialUART Serial1(uart0, PIN_SERIAL1_TX, PIN_SERIAL1_RX);
SerialUART Serial2(uart1, PIN_SERIAL2_TX, PIN_SERIAL2_RX);
SerialUSB Serial;

SerialUART::SerialUART(uart_inst_t* uart, pin_size_t tx, pin_size_t rx,
                       pin_size_t rts, pin_size_t cts)
    : _uart(uart),
      _tx(tx),
      _rx(rx),
      _rts(rts),
      _cts(cts),
      _overflow(false),
      _break(false),
      _invertTX(false),
      _invertRX(false),
      _invertControl(false),
      _queue(nullptr) {
  mutex_init(&_mutex);
  mutex_init(&_fifoMutex);
}

bool SerialUART::setRX(pin_size_t pin) {
  if (_running || !ValidPin(_uart, pin, false)) return false;
  _rx = pin;
  return true;
}

bool SerialUART::setTX(pin_size_t pin) {
  if (_running || !ValidPin(_uart, pin, true)) return false;
  _tx = pin;
  return true;
}

bool SerialUART::setRTS(pin_size_t) { return false; }
bool SerialUART::setCTS(pin_size_t) { return false; }
bool SerialUART::setFIFOSize(size_t size) {
  if (_running || size < 2 || size > 65536) return false;
  _fifoSize = size;
  return true;
}

bool SerialUART::setPollingMode(bool mode) {
  if (_running || !mode) return false;
  _polling = true;
  return true;
}

void SerialUART::begin(unsigned long baud, uint16_t config) {
  CHECK(config == SERIAL_8N1) << "Unimplemented: Arduino-Pico UART format";
  CHECK(!_invertTX && !_invertRX && !_invertControl &&
        _rts == UART_PIN_NOT_DEFINED && _cts == UART_PIN_NOT_DEFINED)
      << "Unimplemented: Arduino-Pico UART inversion/flow control";
  CHECK(ValidPin(_uart, _tx, true) && ValidPin(_uart, _rx, false));
  if (_running) end();
  _actualBaud = uart_init(_uart, baud);
  _baud = baud;
  if (_tx != UART_PIN_NOT_DEFINED) gpio_set_function(_tx, GPIO_FUNC_UART);
  if (_rx != UART_PIN_NOT_DEFINED) gpio_set_function(_rx, GPIO_FUNC_UART);
  _queue = new LocklessQueue<uint8_t>(_fifoSize);
  _polling = true;
  _running = true;
}

void SerialUART::end() {
  if (!_running) return;
  uart_deinit(_uart);
  if (_tx != UART_PIN_NOT_DEFINED) gpio_set_function(_tx, GPIO_FUNC_NULL);
  if (_rx != UART_PIN_NOT_DEFINED) gpio_set_function(_rx, GPIO_FUNC_NULL);
  delete _queue;
  _queue = nullptr;
  _running = false;
}

void SerialUART::_pumpFIFO() {
  if (!_running || _rx == UART_PIN_NOT_DEFINED) return;
  FakeUartDevice& device = FakePico().uartDevice(uart_get_index(_uart), false);
  while (_queue->available() < static_cast<int>(_fifoSize - 1) &&
         device.availableForRead() != 0) {
    uint8_t byte;
    if (device.read(&byte, 1) != 1) break;
    CHECK(_queue->write(byte));
  }
}

int SerialUART::available() {
  CoreMutex operation(&_fifoMutex);
  _pumpFIFO();
  return _running ? _queue->available() : 0;
}

int SerialUART::read() {
  CoreMutex operation(&_fifoMutex);
  _pumpFIFO();
  uint8_t byte;
  return _running && _queue->read(&byte) ? byte : -1;
}

int SerialUART::peek() {
  CoreMutex operation(&_fifoMutex);
  _pumpFIFO();
  uint8_t byte;
  return _running && _queue->peek(&byte) ? byte : -1;
}

int SerialUART::availableForWrite() {
  return _running && _tx != UART_PIN_NOT_DEFINED && uart_is_writable(_uart) ? 1
                                                                            : 0;
}

size_t SerialUART::write(uint8_t byte) { return write(&byte, 1); }
size_t SerialUART::write(const uint8_t* bytes, size_t size) {
  CoreMutex operation(&_mutex);
  if (!_running || _tx == UART_PIN_NOT_DEFINED) return 0;
  uart_write_blocking(_uart, bytes, size);
  return size;
}

void SerialUART::flush() {
  if (_running && _tx != UART_PIN_NOT_DEFINED) uart_tx_wait_blocking(_uart);
}

bool SerialUART::overflow() { return false; }
SerialUART::operator bool() { return _running; }
bool SerialUART::getBreakReceived() {
  LOG(FATAL) << "Unimplemented: Arduino-Pico UART break";
  return false;
}

SerialUSB::SerialUSB() = default;
void SerialUSB::begin(unsigned long) {
  LOG_FIRST_N(WARNING, 1) << "Unimplemented: Pico USB hardware; Serial uses "
                             "the shared host console";
  _running = true;
}

void SerialUSB::end() {
  roo_testing::SchedulerSignalGuard operation;
  _running = false;
  _hostPeek = -1;
}

int SerialUSB::available() {
  roo_testing::SchedulerSignalGuard operation;
  return _running ? (_hostPeek >= 0
                         ? 1
                         : roo_testing::HostConsoleUart().availableForRead())
                  : 0;
}

int SerialUSB::peek() {
  roo_testing::SchedulerSignalGuard operation;
  if (!_running) return -1;
  if (_hostPeek < 0) {
    uint8_t byte;
    if (roo_testing::HostConsoleUart().read(&byte, 1) == 1) _hostPeek = byte;
  }
  return _hostPeek;
}

int SerialUSB::read() {
  roo_testing::SchedulerSignalGuard operation;
  const int byte = peek();
  _hostPeek = -1;
  return byte;
}

int SerialUSB::availableForWrite() { return _running ? 1 : 0; }
size_t SerialUSB::write(uint8_t byte) { return write(&byte, 1); }
size_t SerialUSB::write(const uint8_t* bytes, size_t size) {
  if (!_running) return 0;
  size_t sent = 0;
  while (sent < size)
    sent += roo_testing::HostConsoleUart().write(
        bytes + sent, std::min<size_t>(size - sent, UINT16_MAX));
  return sent;
}

void SerialUSB::flush() {
}  // Descriptor writes already complete without stdio buffering.
SerialUSB::operator bool() { return _running; }
bool SerialUSB::dtr() {
  LOG(FATAL) << "Unimplemented: Pico USB DTR";
  return false;
}

bool SerialUSB::rts() {
  LOG(FATAL) << "Unimplemented: Pico USB RTS";
  return false;
}

void SerialUSB::ignoreFlowControl(bool) {
  LOG(FATAL) << "Unimplemented: Pico USB flow control";
}
