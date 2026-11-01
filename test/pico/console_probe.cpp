#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "hardware/gpio.h"
#include "hardware/uart.h"

// Verifies console operations in bounded subprocesses with pipe-owned
// stdin/out.
int main(int argc, char** argv) {
  if (argc < 2) return 1;
  uart_init(uart0, 115200);
  gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
  gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
  if (strcmp(argv[1], "empty") == 0) {
    return uart_is_readable(uart0) ? 2 : 0;
  }
  if (strcmp(argv[1], "eof") == 0) {
    uart_getc(uart0);
    return 3;
  }
  if (strcmp(argv[1], "flush") == 0) {
    uart_puts(uart0, "ready\n");
    return uart_getc(uart0) == '!' ? 0 : 6;
  }
  if (strcmp(argv[1], "text") == 0) {
    uart_set_translate_crlf(uart0, true);
    uart_puts(uart0, "hello\nworld\r\n");
    return 0;
  }
  if (strcmp(argv[1], "echo") != 0) return 4;
  if (argc != 3) return 5;
  size_t remaining = std::strtoul(argv[2], nullptr, 10);
  std::array<uint8_t, 1024> bytes;
  while (remaining != 0) {
    const size_t chunk = remaining < bytes.size() ? remaining : bytes.size();
    uart_read_blocking(uart0, bytes.data(), chunk);
    uart_write_blocking(uart0, bytes.data(), chunk);
    remaining -= chunk;
  }
  uart_default_tx_wait_blocking();
  return 0;
}
