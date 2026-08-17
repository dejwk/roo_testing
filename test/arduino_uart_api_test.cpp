#include "HardwareSerial.h"
#include "esp32-hal-uart.h"

#include "gtest/gtest.h"

namespace {

TEST(ArduinoUartApi, SupportsCurrentIrdaAndRxPullContracts) {
  constexpr uint8_t kUart = 2;

  EXPECT_FALSE(uartEnableRxInternalPull(SOC_UART_NUM, true));
  EXPECT_TRUE(uartEnableRxInternalPull(kUart, true));

  uart_t* uart = uartBegin(kUart, 115200, SERIAL_8N1, 16, 17, 256, 0,
                           false, 120);
  ASSERT_NE(uart, nullptr);

  EXPECT_FALSE(uartEnableRxInternalPull(kUart, false));
  EXPECT_FALSE(uartSetIrdaDirection(nullptr, ESP32_UART_IRDA_TX));
  EXPECT_FALSE(uartSetIrdaDirection(uart, ESP32_UART_IRDA_TX));
  EXPECT_FALSE(uartSetIrdaDirection(
      uart, static_cast<esp32_uart_irda_direction_t>(2)));

  ASSERT_TRUE(uartSetMode(uart, UART_MODE_IRDA));
  EXPECT_TRUE(uartSetIrdaDirection(uart, ESP32_UART_IRDA_RX));
  EXPECT_TRUE(uartSetIrdaDirection(uart, ESP32_UART_IRDA_TX));

  uartEnd(kUart);
  EXPECT_TRUE(uartEnableRxInternalPull(kUart, false));
}

}  // namespace
