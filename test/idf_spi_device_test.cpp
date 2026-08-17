#include "driver/spi_master.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

// The microcontroller model calls this Arduino UART hook when a fake UART
// source becomes readable. This SPI-only test intentionally does not link the
// Arduino HAL shim.
extern "C" void uart_notify_data_available(uint8_t) {}

namespace {

class RecordingSpiDevice : public SimpleFakeSpiDevice {
 public:
  RecordingSpiDevice() : SimpleFakeSpiDevice("recording IDF SPI device") {}

  void transfer(const FakeSpiInterface& spi, uint8_t* buffer,
                uint16_t bit_count) override {
    selected_during_transfer.push_back(isSelected());
    frequencies.push_back(spi.clkHz());
    modes.push_back(spi.dataMode());
    orders.push_back(spi.bitOrder());
    bit_counts.push_back(bit_count);
    const size_t byte_count = (bit_count + 7) / 8;
    writes.emplace_back(buffer, buffer + byte_count);
    for (size_t i = 0; i < byte_count; ++i) buffer[i] ^= 0xff;
  }

  std::vector<bool> selected_during_transfer;
  std::vector<uint32_t> frequencies;
  std::vector<SpiDataMode> modes;
  std::vector<SpiBitOrder> orders;
  std::vector<uint16_t> bit_counts;
  std::vector<std::vector<uint8_t>> writes;
};

void CountCallback(spi_transaction_t* transaction) {
  ++*static_cast<int*>(transaction->user);
}

TEST(IdfSpiDeviceTest, RoutesTransactionsThroughAttachedFakeDevice) {
  constexpr int kClock = 18;
  constexpr int kMiso = 19;
  constexpr int kMosi = 23;
  constexpr int kChipSelect = 5;

  // FakeEsp32 owns attachment relationships for the lifetime of the process.
  // Keep this device alive accordingly.
  auto* fake_device = new RecordingSpiDevice();
  FakeEsp32().attachSpiDevice(*fake_device, kClock, kMiso, kMosi);
  FakeEsp32().gpio.attachOutput(kChipSelect, fake_device->cs());

  spi_bus_config_t bus{};
  bus.sclk_io_num = kClock;
  bus.miso_io_num = kMiso;
  bus.mosi_io_num = kMosi;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  ASSERT_EQ(ESP_OK, spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED));

  spi_device_interface_config_t config{};
  config.command_bits = 8;
  config.address_bits = 16;
  config.mode = 3;
  config.clock_speed_hz = 8'000'000;
  config.spics_io_num = kChipSelect;
  config.queue_size = 1;
  config.pre_cb = CountCallback;
  config.post_cb = CountCallback;

  spi_device_handle_t device = nullptr;
  ASSERT_EQ(ESP_OK, spi_bus_add_device(SPI2_HOST, &config, &device));
  EXPECT_FALSE(fake_device->isSelected());

  int callback_count = 0;
  spi_transaction_t transaction{};
  transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  transaction.cmd = 0x9f;
  transaction.addr = 0x1234;
  transaction.length = 32;
  transaction.override_freq_hz = 10'000'000;
  transaction.user = &callback_count;
  const std::vector<uint8_t> sent = {0x01, 0x23, 0x45, 0x67};
  std::copy(sent.begin(), sent.end(), transaction.tx_data);

  ASSERT_EQ(ESP_OK, spi_device_polling_transmit(device, &transaction));

  ASSERT_EQ(3U, fake_device->writes.size());
  EXPECT_EQ((std::vector<uint8_t>{0x9f}), fake_device->writes[0]);
  EXPECT_EQ((std::vector<uint8_t>{0x12, 0x34}), fake_device->writes[1]);
  EXPECT_EQ(sent, fake_device->writes[2]);
  EXPECT_EQ((std::vector<uint16_t>{8, 16, 32}), fake_device->bit_counts);
  EXPECT_TRUE(std::all_of(fake_device->selected_during_transfer.begin(),
                          fake_device->selected_during_transfer.end(),
                          [](bool selected) { return selected; }));
  EXPECT_TRUE(std::all_of(fake_device->frequencies.begin(),
                          fake_device->frequencies.end(),
                          [](uint32_t value) { return value == 10'000'000; }));
  EXPECT_TRUE(std::all_of(fake_device->modes.begin(),
                          fake_device->modes.end(),
                          [](SpiDataMode value) { return value == kSpiMode3; }));
  EXPECT_TRUE(std::all_of(fake_device->orders.begin(),
                          fake_device->orders.end(),
                          [](SpiBitOrder value) {
                            return value == kSpiMsbFirst;
                          }));
  EXPECT_EQ((std::vector<uint8_t>{0xfe, 0xdc, 0xba, 0x98}),
            (std::vector<uint8_t>(transaction.rx_data,
                                  transaction.rx_data + 4)));
  EXPECT_EQ(2, callback_count);
  EXPECT_FALSE(fake_device->isSelected());

  EXPECT_EQ(ESP_OK, spi_bus_remove_device(device));
  EXPECT_EQ(ESP_OK, spi_bus_free(SPI2_HOST));
  FakeEsp32().gpio.detach(kChipSelect);
}

}  // namespace
