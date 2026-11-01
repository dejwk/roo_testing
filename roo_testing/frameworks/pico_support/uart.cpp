#include "hardware/uart.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "glog/logging.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

// Native identity tokens; no device registers or host pointers encode indices.
struct uart_inst {
  uint index;
};

namespace {

uart_inst_t uart0_instance{0};
uart_inst_t uart1_instance{1};

/// Rejects arbitrary pointers before dereferencing the opaque handle.
uint Index(uart_inst_t* uart) {
  CHECK(uart == &uart0_instance || uart == &uart1_instance)
      << "Invalid Pico UART instance";
  return uart->index;
}

/// Checks the lifecycle without requiring pins during initial configuration.
uint EnabledIndex(uart_inst_t* uart) {
  const uint index = Index(uart);
  CHECK(FakePico().uartState(index).enabled) << "Pico UART is disabled";
  return index;
}

/// Yields to host device producers without inventing emulated UART data/time.
void WaitForDevice() {
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

}  // namespace

extern "C" {

uart_inst_t* const uart0 = &uart0_instance;
uart_inst_t* const uart1 = &uart1_instance;

uint uart_get_index(uart_inst_t* uart) { return Index(uart); }

uart_inst_t* uart_get_instance(uint index) {
  CHECK_LT(index, 2u) << "Invalid Pico UART index";
  return index == 0 ? uart0 : uart1;
}

bool uart_is_enabled(uart_inst_t* uart) {
  return FakePico().uartState(Index(uart)).enabled;
}

uint uart_init(uart_inst_t* uart, uint baudrate) {
  return FakePico().initUart(Index(uart), baudrate);
}

void uart_deinit(uart_inst_t* uart) { FakePico().deinitUart(Index(uart)); }

uint uart_set_baudrate(uart_inst_t* uart, uint baudrate) {
  return FakePico().setUartBaudrate(Index(uart), baudrate);
}

void uart_set_translate_crlf(uart_inst_t* uart, bool enabled) {
  FakePico().setUartTranslateCrLf(Index(uart), enabled);
}

void uart_set_hw_flow(uart_inst_t* uart, bool cts, bool rts) {
  EnabledIndex(uart);
  CHECK(!cts && !rts) << "Unimplemented: Pico UART hardware flow control";
}

void uart_set_format(uart_inst_t* uart, uint data_bits, uint stop_bits,
                     uart_parity_t parity) {
  EnabledIndex(uart);
  CHECK(data_bits == 8 && stop_bits == 1 && parity == UART_PARITY_NONE)
      << "Unimplemented: Pico UART format other than 8N1";
}

void uart_set_irqs_enabled(uart_inst_t* uart, bool, bool) {
  EnabledIndex(uart);
  LOG(FATAL) << "Unimplemented: Pico UART IRQs";
}

void uart_set_irq_enables(uart_inst_t* uart, bool rx, bool tx) {
  uart_set_irqs_enabled(uart, rx, tx);
}

bool uart_is_writable(uart_inst_t* uart) {
  return FakePico().uartDevice(Index(uart), true).availableForWrite() != 0;
}

bool uart_is_readable(uart_inst_t* uart) {
  return FakePico().uartDevice(Index(uart), false).availableForRead() != 0;
}

bool uart_is_readable_within_us(uart_inst_t* uart, uint32_t us) {
  const absolute_time_t deadline = make_timeout_time_us(us);
  do {
    if (uart_is_readable(uart)) return true;
    const int64_t remaining =
        absolute_time_diff_us(get_absolute_time(), deadline);
    if (remaining <= 0) return false;
    sleep_us(std::min<int64_t>(remaining, 1000));
  } while (true);
}

void uart_write_blocking(uart_inst_t* uart, const uint8_t* src, size_t len) {
  const uint index = EnabledIndex(uart);
  if (len == 0) return;
  CHECK(src != nullptr) << "Invalid Pico UART write buffer";
  FakeUartDevice& device = FakePico().uartDevice(index, true);
  while (len != 0) {
    const uint16_t chunk = std::min<size_t>(len, UINT16_MAX);
    const size_t sent = device.write(src, chunk);
    CHECK_LE(sent, chunk) << "Invalid Pico UART device write count";
    src += sent;
    len -= sent;
    if (sent == 0) WaitForDevice();
  }
}

void uart_read_blocking(uart_inst_t* uart, uint8_t* dst, size_t len) {
  const uint index = EnabledIndex(uart);
  if (len == 0) return;
  CHECK(dst != nullptr) << "Invalid Pico UART read buffer";
  FakeUartDevice& device = FakePico().uartDevice(index, false);
  while (len != 0) {
    const uint16_t chunk = std::min<size_t>(len, UINT16_MAX);
    const size_t received = device.read(dst, chunk);
    CHECK_LE(received, chunk) << "Invalid Pico UART device read count";
    dst += received;
    len -= received;
    if (received == 0) {
      CHECK(!device.inputClosed()) << "Pico UART input reached EOF";
      WaitForDevice();
    }
  }
}

void uart_putc_raw(uart_inst_t* uart, char c) {
  const uint8_t byte = static_cast<uint8_t>(c);
  uart_write_blocking(uart, &byte, 1);
}

void uart_putc(uart_inst_t* uart, char c) {
  const uint index = EnabledIndex(uart);
  if (c == '\n' && FakePico().uartState(index).translate_crlf) {
    uart_putc_raw(uart, '\r');
  }
  uart_putc_raw(uart, c);
}

void uart_puts(uart_inst_t* uart, const char* s) {
  EnabledIndex(uart);
  CHECK(s != nullptr) << "Invalid Pico UART string";
  bool last_was_cr = false;
  while (*s != '\0') {
    if (last_was_cr) {
      uart_putc_raw(uart, *s);
    } else {
      uart_putc(uart, *s);
    }
    last_was_cr = *s++ == '\r';
  }
}

char uart_getc(uart_inst_t* uart) {
  uint8_t byte;
  uart_read_blocking(uart, &byte, 1);
  return static_cast<char>(byte);
}

void uart_set_break(uart_inst_t* uart, bool) {
  EnabledIndex(uart);
  LOG(FATAL) << "Unimplemented: Pico UART break";
}

void uart_tx_wait_blocking(uart_inst_t* uart) {
  FakePico().uartDevice(Index(uart), true);
}

void uart_default_tx_wait_blocking() { uart_tx_wait_blocking(uart_default); }

}  // extern "C"
