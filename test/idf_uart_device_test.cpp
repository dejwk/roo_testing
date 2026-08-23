#include <array>

#include "driver/uart.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/uart/fake_uart.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

TEST(IdfUartDevice, RoutesBytesUsingConfiguredTxAndRxPins) {
  FakeUartCable cable;
  auto& board = FakeEsp32();
  board.attachUartDevice(cable.end_a(), /*tx=*/27, /*rx=*/14);
  board.attachUartDevice(cable.end_b(), /*tx=*/25, /*rx=*/26);

  ASSERT_EQ(
      uart_set_pin(UART_NUM_1, 27, 14, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
      ESP_OK);
  ASSERT_EQ(
      uart_set_pin(UART_NUM_2, 25, 26, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
      ESP_OK);

  constexpr std::array<uint8_t, 4> sent = {1, 2, 3, 4};
  ASSERT_EQ(uart_write_bytes(UART_NUM_1, sent.data(), sent.size()),
            static_cast<int>(sent.size()));

  std::array<uint8_t, sent.size()> received = {};
  ASSERT_EQ(uart_read_bytes(UART_NUM_2, received.data(), received.size(), 0),
            static_cast<int>(received.size()));
  EXPECT_EQ(received, sent);
}

}  // namespace
