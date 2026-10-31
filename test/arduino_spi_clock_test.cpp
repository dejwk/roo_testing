#include <array>
#include <chrono>
#include <cstdint>

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
  }

  uint32_t frequency = 0;
  uint16_t bits = 0;
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

  ClockRecordingSpiDevice device;
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

}  // namespace
