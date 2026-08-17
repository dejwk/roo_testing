#include "esp32-hal-uart.h"
#include "esp32-hal-periman.h"
#include "driver/uart.h"

#include <array>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

struct uart_struct_t {
  uint8_t number;
  bool installed;
  uint32_t baud_rate;
  uint32_t config;
  int8_t rx_pin;
  int8_t tx_pin;
  int8_t cts_pin;
  int8_t rts_pin;
  bool has_peek;
  uint8_t peek;
  uint32_t inversion_mask;
  uart_hw_flowcontrol_t flow_control;
  uart_mode_t mode;
  uart_sclk_t clock_source;
  uint8_t rx_timeout;
  uint8_t fifo_threshold;
  QueueHandle_t event_queue;
};

namespace {
constexpr std::array<uint8_t, SOC_UART_NUM> kTxSignals = {14, 17, 198};
constexpr std::array<uint8_t, SOC_UART_NUM> kRxSignals = {14, 17, 198};
std::array<uart_t, SOC_UART_NUM> buses = [] {
  std::array<uart_t, SOC_UART_NUM> result{};
  for (uint8_t i = 0; i < result.size(); ++i) {
    result[i].number = i;
    result[i].rx_pin = result[i].tx_pin = result[i].cts_pin = result[i].rts_pin = -1;
    result[i].clock_source = UART_SCLK_DEFAULT;
    result[i].mode = UART_MODE_UART;
  }
  return result;
}();
int debug_uart = -1;
bool valid(uint8_t number) { return number < buses.size(); }
}  // namespace

extern "C" {

bool _testUartBegin(uint8_t uart_nr, uint32_t baudrate, uint32_t config,
                    int8_t rx_pin, int8_t tx_pin, uint32_t rx_buffer_size,
                    uint32_t tx_buffer_size, bool inverted,
                    uint8_t fifo_threshold) {
  return uartBegin(uart_nr, baudrate, config, rx_pin, tx_pin, rx_buffer_size,
                   tx_buffer_size, inverted, fifo_threshold) != nullptr;
}

uart_t *uartBegin(uint8_t number, uint32_t baudrate, uint32_t config,
                  int8_t rx_pin, int8_t tx_pin, uint32_t, uint32_t,
                  bool inverted, uint8_t fifo_threshold) {
  if (!valid(number) || baudrate == 0) return nullptr;
  uart_t &uart = buses[number];
  if (uart.event_queue == nullptr) {
    uart.event_queue = xQueueCreate(20, sizeof(uart_event_t));
    if (uart.event_queue == nullptr) return nullptr;
  }
  uart.installed = true;
  uart.baud_rate = baudrate;
  uart.config = config;
  uart.fifo_threshold = fifo_threshold;
  uart.inversion_mask = inverted ? UART_SIGNAL_RXD_INV | UART_SIGNAL_TXD_INV : 0;
  if (!uartSetPins(number, rx_pin, tx_pin, -1, -1)) {
    uart.installed = false;
    return nullptr;
  }
  return &uart;
}

void uartEnd(uint8_t number) {
  if (!valid(number)) return;
  uart_t &uart = buses[number];
  if (uart.rx_pin >= 0) perimanClearPinBus(uart.rx_pin);
  if (uart.tx_pin >= 0) perimanClearPinBus(uart.tx_pin);
  if (uart.cts_pin >= 0) perimanClearPinBus(uart.cts_pin);
  if (uart.rts_pin >= 0) perimanClearPinBus(uart.rts_pin);
  uart.installed = false;
  uart.has_peek = false;
  if (uart.event_queue != nullptr) {
    vQueueDelete(uart.event_queue);
    uart.event_queue = nullptr;
  }
}

void uartGetEventQueue(uart_t *uart, QueueHandle_t *queue) {
  if (queue) *queue = uart && uart->installed ? uart->event_queue : nullptr;
}

uint32_t uartAvailable(uart_t *uart) {
  return uart && uart->installed
             ? FakeEsp32().uart(uart->number).availableForRead() + (uart->has_peek ? 1 : 0)
             : 0;
}
uint32_t uartAvailableForWrite(uart_t *uart) {
  return uart && uart->installed ? FakeEsp32().uart(uart->number).availableForWrite() : 0;
}
size_t uartReadBytes(uart_t *uart, uint8_t *buffer, size_t size, uint32_t) {
  if (!uart || !uart->installed || (!buffer && size)) return 0;
  size_t count = 0;
  if (uart->has_peek && size) {
    buffer[count++] = uart->peek;
    uart->has_peek = false;
  }
  return count + FakeEsp32().uart(uart->number).read(buffer + count, size - count);
}
uint8_t uartRead(uart_t *uart) {
  uint8_t value = 0;
  uartReadBytes(uart, &value, 1, 0);
  return value;
}
uint8_t uartPeek(uart_t *uart) {
  if (!uart || !uart->installed) return 0;
  if (!uart->has_peek) uart->has_peek = FakeEsp32().uart(uart->number).read(&uart->peek, 1) == 1;
  return uart->has_peek ? uart->peek : 0;
}

void uartWrite(uart_t *uart, uint8_t value) { uartWriteBuf(uart, &value, 1); }
void uartWriteBuf(uart_t *uart, const uint8_t *data, size_t length) {
  if (uart && uart->installed && data) FakeEsp32().uart(uart->number).write(data, length);
}
void uartFlush(uart_t *) { FakeEsp32().flush(); }
void uartFlushTxOnly(uart_t *uart, bool) { uartFlush(uart); }

bool uartSetBaudRate(uart_t *uart, uint32_t baudrate) {
  if (!uart || !uart->installed || !baudrate) return false;
  uart->baud_rate = baudrate;
  return true;
}
uint32_t uartGetBaudRate(uart_t *uart) { return uart ? uart->baud_rate : 0; }
bool uartPinSignalInversion(uart_t *uart, uint32_t mask, bool inverted) {
  if (!uart) return false;
  if (inverted) uart->inversion_mask |= mask; else uart->inversion_mask &= ~mask;
  return true;
}
bool uartSetRxInvert(uart_t *uart, bool invert) { return uartPinSignalInversion(uart, UART_SIGNAL_RXD_INV, invert); }
bool uartSetTxInvert(uart_t *uart, bool invert) { return uartPinSignalInversion(uart, UART_SIGNAL_TXD_INV, invert); }
bool uartSetCtsInvert(uart_t *uart, bool invert) { return uartPinSignalInversion(uart, UART_SIGNAL_CTS_INV, invert); }
bool uartSetRtsInvert(uart_t *uart, bool invert) { return uartPinSignalInversion(uart, UART_SIGNAL_RTS_INV, invert); }
bool uartSetRxTimeout(uart_t *uart, uint8_t timeout) { if (!uart) return false; uart->rx_timeout = timeout; return true; }
bool uartSetRxFIFOFull(uart_t *uart, uint8_t threshold) { if (!uart) return false; uart->fifo_threshold = threshold; return true; }
void uartSetFastReading(uart_t *uart) { if (uart) uart->rx_timeout = 0; }
void uartSetDebug(uart_t *uart) { debug_uart = uart ? uart->number : -1; }
int uartGetDebug() { return debug_uart; }
bool uartIsDriverInstalled(uart_t *uart) { return uart && uart->installed; }

bool uartSetPins(uint8_t number, int8_t rx_pin, int8_t tx_pin,
                 int8_t cts_pin, int8_t rts_pin) {
  if (!valid(number)) return false;
  uart_t &uart = buses[number];
  if (rx_pin >= 0) {
    if (uart.rx_pin >= 0 && uart.rx_pin != rx_pin) perimanClearPinBus(uart.rx_pin);
    uart.rx_pin = rx_pin;
    FakeEsp32().in_matrix.assign(rx_pin, kRxSignals[number], false);
    perimanSetPinBus(rx_pin, ESP32_BUS_TYPE_UART_RX, &uart, number, -1);
  }
  if (tx_pin >= 0) {
    if (uart.tx_pin >= 0 && uart.tx_pin != tx_pin) perimanClearPinBus(uart.tx_pin);
    uart.tx_pin = tx_pin;
    FakeEsp32().out_matrix.assign(tx_pin, kTxSignals[number], false, false);
    perimanSetPinBus(tx_pin, ESP32_BUS_TYPE_UART_TX, &uart, number, -1);
  }
  if (cts_pin >= 0) {
    uart.cts_pin = cts_pin;
    perimanSetPinBus(cts_pin, ESP32_BUS_TYPE_UART_CTS, &uart, number, -1);
  }
  if (rts_pin >= 0) {
    uart.rts_pin = rts_pin;
    perimanSetPinBus(rts_pin, ESP32_BUS_TYPE_UART_RTS, &uart, number, -1);
  }
  return true;
}

int8_t uart_get_RxPin(uint8_t number) { return valid(number) ? buses[number].rx_pin : -1; }
int8_t uart_get_TxPin(uint8_t number) { return valid(number) ? buses[number].tx_pin : -1; }
bool uartSetHwFlowCtrlMode(uart_t *uart, uart_hw_flowcontrol_t mode, uint8_t) { if (!uart) return false; uart->flow_control = mode; return true; }
bool uartSetMode(uart_t *uart, uart_mode_t mode) { if (!uart) return false; uart->mode = mode; return true; }
bool uartSetClockSource(uint8_t number, uart_sclk_t source) { if (!valid(number)) return false; buses[number].clock_source = source; return true; }
void uartStartDetectBaudrate(uart_t *) {}
unsigned long uartDetectBaudrate(uart_t *uart) { return uart ? uart->baud_rate : 0; }
void uart_internal_loopback(uint8_t number, int8_t rx_pin) { if (valid(number)) uartSetPins(number, rx_pin, buses[number].tx_pin, -1, -1); }
void uart_internal_hw_flow_ctrl_loopback(uint8_t number, int8_t cts_pin) { if (valid(number)) uartSetPins(number, -1, -1, cts_pin, cts_pin); }
void uart_send_break(uint8_t) {}
int uart_send_msg_with_break(uint8_t number, uint8_t *message, size_t size) {
  if (!valid(number) || !buses[number].installed) return -1;
  uartWriteBuf(&buses[number], message, size);
  return static_cast<int>(size);
}
uint16_t uart_get_max_rx_timeout(uint8_t) { return 127; }
void uart_notify_data_available(uint8_t number) {
  if (!valid(number)) return;
  uart_t &uart = buses[number];
  if (!uart.installed || uart.event_queue == nullptr) return;
  uart_event_t event{};
  event.type = UART_DATA;
  event.size = FakeEsp32().uart(number).availableForRead();
  event.timeout_flag = true;
  xQueueSend(uart.event_queue, &event, 0);
}

}  // extern "C"
