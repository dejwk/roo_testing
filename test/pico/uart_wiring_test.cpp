#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {

// Verifies optional directions, nonadjacent pin groups, and immutable wiring.
// A separate executable leaves pins untouched until fixture attachment checks.
TEST(PicoUartWiringTest, ValidatesTopologyBeforeApplicationSetup) {
  static FakeUartCable loopback;
  static FakeUartCable alternate;
  static roo_testing_transducers::ConstVoltage source(0.0f);
  EXPECT_DEATH(FakePico().attachUartDevice(loopback.end_a(), 25, -1), "TX pin");
  EXPECT_DEATH(FakePico().attachUartDevice(loopback.end_a(), -1, 24), "RX pin");
  EXPECT_DEATH(FakePico().attachUartDevice(loopback.end_a(), 24, 29),
               "controller mismatch");
  FakePico().attachUartDevice(loopback.end_a(), 24, -1);
  FakePico().attachUartDevice(loopback.end_b(), -1, 25);
  FakePico().attachUartDevice(alternate.end_a(), 8, 21);
  FakePico().attachInput(28, source);
  EXPECT_DEATH(FakePico().attachUartDevice(loopback.end_a(), 12, 13),
               "already attached");
  EXPECT_DEATH(FakePico().attachUartDevice(alternate.end_b(), 24, -1),
               "attachment conflict");
  EXPECT_DEATH(gpio_set_function(28, GPIO_FUNC_UART), "GPIO attachment");
  gpio_init(20);
  EXPECT_DEATH(FakePico().attachUartDevice(alternate.end_b(), 20, -1),
               "before configuring");

  uart_init(uart1, 115200);
  gpio_set_function(24, GPIO_FUNC_UART);
  gpio_set_function(25, GPIO_FUNC_UART);
  uart_putc_raw(uart1, 'L');
  EXPECT_EQ(uart_getc(uart1), 'L');
  gpio_set_function(24, GPIO_FUNC_NULL);
  gpio_set_function(25, GPIO_FUNC_NULL);
  gpio_set_function(8, GPIO_FUNC_UART);
  gpio_set_function(21, GPIO_FUNC_UART);
  uart_putc_raw(uart1, 'T');
  uint8_t byte;
  ASSERT_EQ(alternate.end_b().read(&byte, 1), 1u);
  EXPECT_EQ(byte, 'T');
  byte = 'R';
  ASSERT_EQ(alternate.end_b().write(&byte, 1), 1u);
  EXPECT_EQ(uart_getc(uart1), 'R');
}

}  // namespace
