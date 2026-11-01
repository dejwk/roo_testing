#include "hardware/spi.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {

/// Records shared wire transactions and replies with a deterministic XOR.
class RecordingDevice : public SimpleFakeSpiDevice {
 public:
  /// Creates a selected-device observer for the expected controller.
  explicit RecordingDevice(uint index)
      : SimpleFakeSpiDevice("capture"), index_(index) {}

  /// Captures byte order, CS, controller settings, busy state, and chunk time.
  void transfer(const FakeSpiInterface& spi, uint8_t* bytes,
                uint16_t bits) override {
    EXPECT_TRUE(isSelected());
    EXPECT_TRUE(spi_is_busy(spi_get_instance(index_)));
    EXPECT_EQ(spi.bitOrder(), kSpiMsbFirst);
    sizes.push_back(bits);
    starts.push_back(time_us_64());
    modes.push_back(spi.dataMode());
    clocks.push_back(spi.clkHz());
    sent.insert(sent.end(), bytes, bytes + bits / 8);
    if (mutate) gpio_put(1, false);
    if (nested) {
      const uint8_t byte = 0;
      EXPECT_EQ(spi_write_blocking(spi_get_instance(1 - index_), &byte, 1),
                PICO_ERROR_RESOURCE_IN_USE);
    }
    for (uint i = 0; i < bits / 8; ++i) bytes[i] ^= 0xa5;
  }

  /// Resets observations while retaining the device's borrowed wiring.
  void clear() {
    sent.clear();
    sizes.clear();
    starts.clear();
    modes.clear();
    clocks.clear();
    mutate = false;
    nested = false;
  }

  std::vector<uint8_t> sent;
  std::vector<uint16_t> sizes;
  std::vector<uint64_t> starts;
  std::vector<SpiDataMode> modes;
  std::vector<uint32_t> clocks;
  bool mutate = false;
  bool nested = false;

 private:
  uint index_;
};

/// Attaches all legal complete groups before any SDK pin configuration.
struct Wiring {
  Wiring() : second(0) {
    for (uint group = 0; group < devices.size(); ++group) {
      devices[group] = std::make_unique<RecordingDevice>((group / 2) % 2);
      const uint base = group * 4;
      FakePico().attachSpiDevice(*devices[group], base + 2, base, base + 3);
      FakePico().attachOutput(base + 1, devices[group]->cs());
    }
    FakePico().attachSpiDevice(second, 2, 0, 3);
    FakePico().attachOutput(29, second.cs());
  }

  std::array<std::unique_ptr<RecordingDevice>, 7> devices;
  RecordingDevice second;
};

Wiring& Connections() {
  static Wiring wiring;
  return wiring;
}

/// Selects one SPI group and asserts its software chip-select signal.
void Select(uint group) {
  for (uint pin = 0; pin < 29; ++pin) {
    if (pin % 4 != 1) gpio_set_function(pin, GPIO_FUNC_NULL);
  }
  const uint base = group * 4;
  for (uint pin : {base, base + 2, base + 3}) {
    gpio_set_function(pin, GPIO_FUNC_SPI);
  }
  gpio_put(base + 1, false);
}

class PicoSpiTest : public testing::Test {
 protected:
  void SetUp() override {
    Wiring& wiring = Connections();
    for (uint group = 0; group < wiring.devices.size(); ++group) {
      wiring.devices[group]->clear();
      const uint cs = group * 4 + 1;
      gpio_init(cs);
      gpio_put(cs, true);
      gpio_set_dir(cs, GPIO_OUT);
    }
    wiring.second.clear();
    gpio_init(29);
    gpio_put(29, true);
    gpio_set_dir(29, GPIO_OUT);
    for (uint pin = 0; pin < 29; ++pin) {
      if (pin % 4 != 1) gpio_set_function(pin, GPIO_FUNC_NULL);
    }
    spi_init(spi0, 1000000);
    spi_init(spi1, 1000000);
  }
};

// Verifies checked identities, pinned divider values, reset defaults, and
// errors.
TEST_F(PicoSpiTest, LifecycleBaudrateAndUnsupportedModes) {
  EXPECT_NE(spi0, nullptr);
  EXPECT_NE(spi0, spi1);
  EXPECT_EQ(spi_get_instance(0), spi0);
  EXPECT_EQ(spi_get_index(spi1), 1u);
  EXPECT_EQ(spi_default, spi0);
  EXPECT_EQ(spi_init(spi0, 1000000), 992063u);
  EXPECT_EQ(spi_set_baudrate(spi0, 125000000), 62500000u);
  EXPECT_EQ(spi_get_baudrate(spi1), 992063u);
  spi_set_format(spi0, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  spi_init(spi0, 1000000);
  EXPECT_EQ(FakePico().spiState(0).data_bits, 8u);
  EXPECT_EQ(FakePico().spiState(0).mode, kSpiMode0);
  spi_set_slave(spi0, false);
  EXPECT_DEATH(spi_init(nullptr, 1000000), "instance");
  EXPECT_DEATH(spi_get_index(reinterpret_cast<spi_inst_t*>(123)), "instance");
  EXPECT_DEATH(spi_get_instance(2), "index");
  EXPECT_DEATH(spi_set_baudrate(spi0, 0), "baudrate");
  EXPECT_DEATH(spi_set_baudrate(spi0, 1), "too low");
  EXPECT_DEATH(spi_set_baudrate(spi0, UINT32_MAX), "baudrate");
  EXPECT_DEATH(spi_set_format(spi0, 7, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST),
               "width");
  EXPECT_DEATH(spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_LSB_FIRST),
               "LSB-first");
  EXPECT_DEATH(spi_set_format(spi0, 8, static_cast<spi_cpol_t>(2), SPI_CPHA_0,
                              SPI_MSB_FIRST),
               "polarity");
  EXPECT_DEATH(spi_set_slave(spi0, true), "slave mode");
  EXPECT_DEATH(spi_is_readable(spi0), "FIFO");
  EXPECT_DEATH(spi_is_writable(spi0), "FIFO");
  const uint8_t byte = 1;
  spi_deinit(spi0);
  EXPECT_EQ(spi_write_blocking(spi0, &byte, 1), PICO_ERROR_INVALID_STATE);
}

// Verifies every complete GP0..27 group maps to its RP2040 SPI controller.
TEST_F(PicoSpiTest, AllPhysicalPinGroupsAndClockModes) {
  for (uint group = 0; group < 7; ++group) {
    Select(group);
    spi_inst_t* spi = spi_get_instance((group / 2) % 2);
    const uint mode = group % 4;
    spi_set_format(spi, 8, static_cast<spi_cpol_t>(mode / 2),
                   static_cast<spi_cpha_t>(mode % 2), SPI_MSB_FIRST);
    const uint8_t tx[] = {0x12, 0xa0};
    uint8_t rx[2];
    ASSERT_EQ(spi_write_read_blocking(spi, tx, rx, 2), 2);
    EXPECT_EQ(rx[0], 0xb7);
    EXPECT_EQ(rx[1], 0x05);
    RecordingDevice& device = *Connections().devices[group];
    EXPECT_EQ(device.sent, std::vector<uint8_t>({0x12, 0xa0}));
    EXPECT_EQ(device.modes.back(), static_cast<SpiDataMode>(mode));
    EXPECT_EQ(device.clocks.back(), spi_get_baudrate(spi));
    EXPECT_FALSE(spi_is_busy(spi));
    gpio_put(group * 4 + 1, true);
  }
}

// Verifies halfwords serialize MSB first independently of native host
// endianness, including full-duplex, in-place, repeated dummy words, and
// write-only calls.
TEST_F(PicoSpiTest, SixTransferApisAndWordOrder) {
  Select(0);
  uint8_t bytes[] = {0x12, 0x80};
  EXPECT_EQ(spi_write_read_blocking(spi0, bytes, bytes, 2), 2);
  EXPECT_EQ(bytes[0], 0xb7);
  EXPECT_EQ(bytes[1], 0x25);
  EXPECT_EQ(spi_read_blocking(spi0, 0xff, bytes, 2), 2);
  EXPECT_EQ(bytes[0], 0x5a);
  EXPECT_EQ(bytes[1], 0x5a);
  EXPECT_EQ(spi_write_blocking(spi0, bytes, 2), 2);
  Connections().devices[0]->clear();
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  uint16_t words[] = {0x1234, 0x8001, 0x00ff};
  EXPECT_EQ(spi_write16_read16_blocking(spi0, words, words, 3), 3);
  EXPECT_EQ(words[0], 0xb791);
  EXPECT_EQ(words[1], 0x25a4);
  EXPECT_EQ(words[2], 0xa55a);
  EXPECT_EQ(Connections().devices[0]->sent,
            std::vector<uint8_t>({0x12, 0x34, 0x80, 1, 0, 0xff}));
  EXPECT_EQ(spi_read16_blocking(spi0, 0x0102, words, 3), 3);
  EXPECT_EQ(words[2], 0xa4a7);
  EXPECT_EQ(spi_write16_blocking(spi0, words, 3), 3);
}

// Verifies byte-count overflow, buffer/format errors, and partial overlap leave
// destinations, device observations, and manual time untouched.
TEST_F(PicoSpiTest, InvalidTransfersHaveNoEffects) {
  Select(0);
  uint8_t bytes[] = {1, 2, 3, 4};
  const uint64_t before = time_us_64();
  EXPECT_EQ(spi_write_blocking(spi0, nullptr, 1), PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(spi_read_blocking(spi0, 0, nullptr, 1), PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(spi_write_read_blocking(spi0, bytes, bytes + 1, 3),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(spi_write_blocking(spi0, bytes, size_t{INT_MAX} + 1),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(spi_write16_blocking(spi0, reinterpret_cast<uint16_t*>(bytes), 1),
            PICO_ERROR_INVALID_ARG);
  EXPECT_EQ(spi_write_blocking(spi0, nullptr, 0), 0);
  EXPECT_EQ(bytes[1], 2);
  EXPECT_TRUE(Connections().devices[0]->sent.empty());
  EXPECT_EQ(time_us_64(), before);
}

// Verifies large, chunked transfers retain CS and data boundaries, while
// elapsed time follows total bits rather than rounding each chunk to
// microseconds.
TEST_F(PicoSpiTest, ChunkingAndTransferDuration) {
  Select(0);
  const uint baud = spi_set_baudrate(spi0, 10416666);
  std::vector<uint8_t> data(70001);
  for (size_t i = 0; i < data.size(); ++i) data[i] = i;
  std::vector<uint8_t> rx(data.size());
  const uint64_t before = time_us_64();
  ASSERT_EQ(spi_write_read_blocking(spi0, data.data(), rx.data(), data.size()),
            70001);
  const uint64_t ns =
      (data.size() * uint64_t{8} * 1000000000 + baud - 1) / baud;
  EXPECT_NEAR(time_us_64() - before, ns / 1000.0, 1.0);
  RecordingDevice& device = *Connections().devices[0];
  EXPECT_EQ(device.sent, data);
  EXPECT_EQ(device.sizes.size(), (data.size() + 255) / 256);
  for (size_t i = 0; i < rx.size(); ++i) {
    EXPECT_EQ(rx[i], static_cast<uint8_t>(data[i] ^ 0xa5));
  }
  for (size_t i = 0; i < device.sizes.size(); ++i) {
    EXPECT_LE(device.sizes[i], 2048);
    EXPECT_EQ(device.sizes[i] % 8, 0);
    const uint64_t bits_before = i * uint64_t{256} * 8;
    EXPECT_NEAR(device.starts[i] - before,
                (bits_before * 1000000000 + baud - 1) / baud / 1000.0, 1.0);
  }
  device.clear();
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  std::vector<uint16_t> words(129, 0x1234);
  EXPECT_EQ(spi_write16_blocking(spi0, words.data(), words.size()), 129);
  EXPECT_EQ(device.sizes, std::vector<uint16_t>({2048, 16}));
}

// Verifies software CS selection, broadcast TX isolation, responder conflicts,
// and undriven MISO sampling via its configured weak pull.
TEST_F(PicoSpiTest, ChipSelectAndMultipleDevices) {
  Select(0);
  const uint8_t byte = 0x42;
  gpio_put(1, true);
  uint8_t rx = 99;
  EXPECT_EQ(spi_read_blocking(spi0, 0xff, &rx, 1), 1);
  EXPECT_EQ(rx, 0);
  EXPECT_TRUE(Connections().devices[0]->sent.empty());
  gpio_pull_up(0);
  EXPECT_EQ(spi_read_blocking(spi0, 0, &rx, 1), 1);
  EXPECT_EQ(rx, 0xff);
  gpio_disable_pulls(0);
  EXPECT_EQ(spi_read_blocking(spi0, 0, &rx, 1), PICO_ERROR_INVALID_STATE);
  gpio_pull_down(0);
  gpio_put(1, false);
  gpio_put(29, false);
  const uint64_t before = time_us_64();
  EXPECT_EQ(spi_write_read_blocking(spi0, &byte, &rx, 1),
            PICO_ERROR_RESOURCE_IN_USE);
  EXPECT_EQ(time_us_64(), before);
  EXPECT_EQ(rx, 0xff);
  EXPECT_TRUE(Connections().second.sent.empty());
  EXPECT_EQ(spi_write_blocking(spi0, &byte, 1), 1);
  EXPECT_EQ(Connections().devices[0]->sent, std::vector<uint8_t>({byte}));
  EXPECT_EQ(Connections().second.sent, std::vector<uint8_t>({byte}));
}

// Verifies invalid/ambiguous functions and input overrides do not reach
// devices; callback reconfiguration is rejected and nested transfers report
// busy state.
TEST_F(PicoSpiTest, PinErrorsAndReentrancy) {
  Select(0);
  const uint8_t byte = 1;
  uint8_t rx = 99;
  gpio_set_function(6, GPIO_FUNC_SPI);
  EXPECT_EQ(spi_write_blocking(spi0, &byte, 1), PICO_ERROR_INVALID_STATE);
  gpio_set_function(6, GPIO_FUNC_NULL);
  gpio_set_input_enabled(0, false);
  EXPECT_EQ(spi_read_blocking(spi0, 0, &rx, 1), PICO_ERROR_INVALID_STATE);
  gpio_set_input_enabled(0, true);
  gpio_set_inover(0, GPIO_OVERRIDE_INVERT);
  EXPECT_EQ(spi_read_blocking(spi0, 0, &rx, 1), PICO_ERROR_INVALID_STATE);
  EXPECT_EQ(rx, 99);
  EXPECT_DEATH(gpio_set_function(1, GPIO_FUNC_SPI), "SPI CSn");
  EXPECT_DEATH(FakePico().padVoltage(2), "SPI pin voltage");
  EXPECT_DEATH(
      {
        gpio_init(2);
        gpio_set_dir(2, GPIO_OUT);
      },
      "SIO output");
  Connections().devices[0]->mutate = true;
  EXPECT_DEATH(spi_write_blocking(spi0, &byte, 1),
               "reconfiguration during SPI");
  Connections().devices[0]->mutate = false;
  Connections().devices[0]->nested = true;
  EXPECT_EQ(spi_write_blocking(spi0, &byte, 1), 1);
}

}  // namespace
