#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {

/// Models a byte echo without introducing an additional bus abstraction.
class Echo : public SimpleFakeSpiDevice {
 public:
  /// Creates a selected fixture with no GPIO CS attachment.
  Echo() : SimpleFakeSpiDevice("echo") { cs().write(0.0f); }

  /// Leaves outgoing bytes unchanged as the simulated incoming response.
  void transfer(const FakeSpiInterface&, uint8_t*, uint16_t) override {}
};

// Verifies role/controller/attachment validation on initially untouched pins,
// including GP28 MISO, nonadjacent groups, and write-only devices.
TEST(PicoSpiWiringTest, ChecksWiringBeforeConfiguration) {
  static Echo first;
  static Echo second;
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 30, 28, 27), "GPIO pin");
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 27, 28, 26), "SCK pin");
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 26, 28, 26), "TX pin");
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 26, 27, 27), "RX pin");
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 26, 0, 27),
               "controller mismatch");
  FakePico().attachSpiDevice(first, 26, 28, 15);
  FakePico().attachSpiDevice(second, 2, -1, 3);
  EXPECT_DEATH(FakePico().attachSpiDevice(first, 10, 8, 11),
               "already attached");
  EXPECT_DEATH(FakePico().attachOutput(26, second.cs()), "attachment conflict");
  gpio_set_function(26, GPIO_FUNC_SPI);
  gpio_set_function(28, GPIO_FUNC_SPI);
  gpio_set_function(15, GPIO_FUNC_SPI);
  spi_init(spi1, 1000000);
  const uint8_t tx = 0x5a;
  uint8_t rx;
  EXPECT_EQ(spi_write_read_blocking(spi1, &tx, &rx, 1), 1);
  EXPECT_EQ(rx, tx);
  gpio_set_function(2, GPIO_FUNC_SPI);
  gpio_set_function(3, GPIO_FUNC_SPI);
  spi_init(spi0, 1000000);
  EXPECT_EQ(spi_write_blocking(spi0, &tx, 1), 1);
  EXPECT_EQ(spi_read_blocking(spi0, 0, &rx, 1), PICO_ERROR_INVALID_STATE);
  EXPECT_DEATH(FakePico().attachSpiDevice(second, 2, -1, 3),
               "before configuring");
}

}  // namespace
