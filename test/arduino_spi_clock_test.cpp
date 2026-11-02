#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include "Arduino.h"
#include "SPI.h"
#include "gtest/gtest.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

class ClockRecordingSpiDevice : public SimpleFakeSpiDevice {
 public:
  ClockRecordingSpiDevice() : SimpleFakeSpiDevice("Arduino SPI clock test") {}

  void transfer(const FakeSpiInterface& spi, uint8_t*,
                uint16_t bit_count) override {
    frequency = spi.clkHz();
    bits = bit_count;
    mode = spi.dataMode();
    order = spi.bitOrder();
  }

  uint32_t frequency = 0;
  uint16_t bits = 0;
  SpiDataMode mode = kSpiMode0;
  SpiBitOrder order = kSpiMsbFirst;
  std::vector<bool> selection_edges;

 protected:
  void onChipSelect(bool selected) override {
    selection_edges.push_back(selected);
  }
};

TEST(ArduinoSpiClockTest, MatchesUpstreamDividedClockEncoding) {
  constexpr uint32_t kFrequency = 20'000'000;
  const uint32_t divider = spiFrequencyToClockDiv(nullptr, kFrequency);
  EXPECT_EQ(divider, 0x00041001U);
  EXPECT_EQ(spiClockDivToFrequency(nullptr, divider), kFrequency);

  constexpr uint32_t kNonDivisibleFrequency = 26'000'000;
  const uint32_t non_divisible_divider =
      spiFrequencyToClockDiv(nullptr, kNonDivisibleFrequency);
  EXPECT_LE(spiClockDivToFrequency(nullptr, non_divisible_divider),
            kNonDivisibleFrequency);

  constexpr int kClock = 4;
  constexpr int kMiso = 5;
  constexpr int kMosi = 6;
  constexpr int kChipSelect = 7;

  static ClockRecordingSpiDevice device;
  FakeEsp32().attachSpiDevice(device, kClock, kMiso, kMosi);
  FakeEsp32().gpio.attachOutput(kChipSelect, device.cs());

#if CONFIG_IDF_TARGET_ESP32
  SPIClass spi(VSPI);
#else
  SPIClass spi(FSPI);
#endif
  ASSERT_TRUE(spi.begin(kClock, kMiso, kMosi, kChipSelect));
  pinMode(kChipSelect, OUTPUT);
  digitalWrite(kChipSelect, LOW);

  std::array<uint8_t, 64> data{};
  spi.beginTransaction(SPISettings(kFrequency, SPI_MSBFIRST, SPI_MODE0));
  const auto start = std::chrono::steady_clock::now();
  spi.writeBytes(data.data(), data.size());
  const auto elapsed = std::chrono::steady_clock::now() - start;
  spi.endTransaction();

  EXPECT_EQ(device.frequency, kFrequency);
  EXPECT_EQ(device.bits, 512);
  EXPECT_LT(elapsed, std::chrono::seconds(2));

  digitalWrite(kChipSelect, HIGH);
  spi.end();
  FakeEsp32().gpio.detach(kChipSelect);
}

// Verifies register-backed Arduino SPI reports the selected mode and bit
// order accurately to protocol-checking devices on every transaction.
TEST(ArduinoSpiClockTest, ReportsAllModesAndBothBitOrders) {
  static ClockRecordingSpiDevice device;
  // GPIO 21 exists on ESP32, S3 and C3; GPIO 23 is absent on S3/C3.
  FakeEsp32().attachSpiDevice(device, 18, 19, 21);
  FakeEsp32().gpio.attachOutput(5, device.cs());
#if CONFIG_IDF_TARGET_ESP32
  SPIClass spi(VSPI);
#else
  SPIClass spi(FSPI);
#endif
  ASSERT_TRUE(spi.begin(18, 19, 21));
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);
  device.selection_edges.clear();
  digitalWrite(5, LOW);
  digitalWrite(5, LOW);  // Repeating a level is not a new frame.
  const uint8_t modes[] = {SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3};
  for (int mode = 0; mode < 4; ++mode) {
    for (uint8_t order : {SPI_MSBFIRST, SPI_LSBFIRST}) {
      spi.beginTransaction(SPISettings(1000000, order, modes[mode]));
      spi.transfer(0x96);
      spi.endTransaction();
      EXPECT_EQ(static_cast<SpiDataMode>(mode), device.mode);
      EXPECT_EQ(order == SPI_MSBFIRST ? kSpiMsbFirst : kSpiLsbFirst,
                device.order);
    }
  }
  digitalWrite(5, HIGH);
  EXPECT_EQ((std::vector<bool>{true, false}), device.selection_edges);
  spi.end();
  FakeEsp32().gpio.detach(5);
}

}  // namespace
