#include <array>

#include "driver/uart.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

TEST(IdfUartDevice, RoutesBytesUsingConfiguredTxAndRxPins) {
  FakeUartCable cable;
  auto& board = FakeEsp32();
  board.attachUartDevice(cable.end_a(), /*tx=*/4, /*rx=*/5);
  board.attachUartDevice(cable.end_b(), /*tx=*/6, /*rx=*/7);

  ASSERT_EQ(
      uart_set_pin(UART_NUM_0, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
      ESP_OK);
  ASSERT_EQ(
      uart_set_pin(UART_NUM_1, 6, 7, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
      ESP_OK);

  constexpr std::array<uint8_t, 4> sent = {1, 2, 3, 4};
  ASSERT_EQ(uart_write_bytes(UART_NUM_0, sent.data(), sent.size()),
            static_cast<int>(sent.size()));

  std::array<uint8_t, sent.size()> received = {};
  ASSERT_EQ(uart_read_bytes(UART_NUM_1, received.data(), received.size(), 0),
            static_cast<int>(received.size()));
  EXPECT_EQ(received, sent);
}

}  // namespace
