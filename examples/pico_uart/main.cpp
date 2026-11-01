#include <array>
#include <cstdio>

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

// A bounded SDK application: UART0 sends to UART1, which replies over a cable.
int main() {
  FakeUartCable cable;
  FakePico().attachUartDevice(cable.end_a(), 0, 1);
  FakePico().attachUartDevice(cable.end_b(), 4, 5);
  uart_init(uart0, 115200);
  uart_init(uart1, 115200);
  for (uint pin : {0u, 1u, 4u, 5u}) gpio_set_function(pin, GPIO_FUNC_UART);

  uart_puts(uart0, "ping");
  if (!uart_is_readable_within_us(uart1, 1000)) return 1;
  std::array<uint8_t, 4> request;
  uart_read_blocking(uart1, request.data(), request.size());
  if (request != std::array<uint8_t, 4>{'p', 'i', 'n', 'g'}) return 2;

  uart_puts(uart1, "pong");
  if (!uart_is_readable_within_us(uart0, 1000)) return 3;
  std::array<uint8_t, 4> reply;
  uart_read_blocking(uart0, reply.data(), reply.size());
  if (reply != std::array<uint8_t, 4>{'p', 'o', 'n', 'g'}) return 4;
  uart_deinit(uart0);
  uart_deinit(uart1);
  puts("Pico UART cable: ping -> pong");
  return 0;
}
