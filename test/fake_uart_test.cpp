#include "roo_testing/buses/uart/fake_uart.h"

#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

int notified_uart = -1;

}  // namespace

extern "C" void uart_notify_data_available(uint8_t uart) {
  notified_uart = uart;
}

namespace {

TEST(FakeUartCable, TransfersInBothDirectionsAndNotifiesTheReceiver) {
  FakeUartCable cable;
  int end_a_notifications = 0;
  int end_b_notifications = 0;
  cable.end_a().setDataAvailableFn(
      [&](const FakeUartDevice*) { ++end_a_notifications; });
  cable.end_b().setDataAvailableFn(
      [&](const FakeUartDevice*) { ++end_b_notifications; });

  const uint8_t from_a[] = {1, 2, 3};
  EXPECT_EQ(cable.end_a().write(from_a, sizeof(from_a)), sizeof(from_a));
  EXPECT_EQ(end_a_notifications, 0);
  EXPECT_EQ(end_b_notifications, 1);
  EXPECT_EQ(cable.end_b().availableForRead(), sizeof(from_a));

  uint8_t received[3] = {};
  EXPECT_EQ(cable.end_b().read(received, sizeof(received)), sizeof(received));
  EXPECT_EQ(std::vector<uint8_t>(received, received + sizeof(received)),
            std::vector<uint8_t>(from_a, from_a + sizeof(from_a)));

  const uint8_t from_b[] = {4, 5};
  EXPECT_EQ(cable.end_b().write(from_b, sizeof(from_b)), sizeof(from_b));
  EXPECT_EQ(end_a_notifications, 1);
  EXPECT_EQ(end_b_notifications, 1);
  EXPECT_EQ(cable.end_a().read(received, sizeof(received)), sizeof(from_b));
  EXPECT_EQ(std::vector<uint8_t>(received, received + sizeof(from_b)),
            std::vector<uint8_t>(from_b, from_b + sizeof(from_b)));
}

TEST(FakeEsp32Uart, DataNotificationUsesTheReceivingPin) {
  FakeUartCable cable;
  auto& board = FakeEsp32();
  board.attachUartDevice(cable.end_b(), /*tx=*/-1, /*rx=*/26);
  board.in_matrix.assign(26, board.uart(2).rx_signal(), false);

  notified_uart = -1;
  const uint8_t data = 42;
  ASSERT_EQ(cable.end_a().write(&data, 1), size_t{1});
  EXPECT_EQ(notified_uart, 2);
}

}  // namespace
