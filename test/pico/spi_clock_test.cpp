#include <cstdint>
#include <limits>

#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"

namespace {

/// Counts device delivery without introducing response data or extra timing.
class Device : public SimpleFakeSpiDevice {
 public:
  /// Creates a fixture selected independently of application GPIO CS.
  Device() : SimpleFakeSpiDevice("clock") { cs().write(0.0f); }

  /// Records that the transfer reached the existing device boundary.
  void transfer(const FakeSpiInterface&, uint8_t*, uint16_t) override {
    ++calls;
  }

  int calls = 0;
};

// Verifies auto-mode bus duration reaches the shared clock; manual-mode waits
// near the clock ceiling fail before buffer/device/time mutation.
TEST(PicoSpiClockTest, ValidatesAndPublishesTransferDuration) {
  static Device device;
  FakePico().attachSpiDevice(device, 2, 0, 3);
  spi_init(spi0, 1000000);
  for (uint pin : {0u, 2u, 3u}) gpio_set_function(pin, GPIO_FUNC_SPI);
  const uint8_t tx[] = {1, 2, 3, 4};
  uint8_t rx[] = {99, 99, 99, 99};
#if PICO_SPI_AUTO
  ASSERT_TRUE(system_time_is_auto_sync_enabled());
  const uint64_t before = time_us_64();
  EXPECT_EQ(spi_write_read_blocking(spi0, tx, rx, 4), 4);
  EXPECT_GE(time_us_64() - before, 32u);
  EXPECT_EQ(device.calls, 1);
#else
  ASSERT_FALSE(system_time_is_auto_sync_enabled());
  const uint64_t limit = std::numeric_limits<int64_t>::max();
  system_time_lag_ns(limit - 2000);
  const uint64_t before = time_us_64();
  EXPECT_EQ(spi_write_read_blocking(spi0, tx, rx, 4), PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(time_us_64(), before);
  EXPECT_EQ(device.calls, 0);
  for (uint8_t byte : rx) EXPECT_EQ(byte, 99);
  EXPECT_FALSE(spi_is_busy(spi0));
#endif
}

}  // namespace
