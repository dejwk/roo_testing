#include "roo_testing/devices/storage/sd_spi/sd_spi.h"

#include <array>
#include <vector>

#include "gtest/gtest.h"

namespace {
class Settings : public FakeSpiInterface {
 public:
  Settings() : FakeSpiInterface("SD probe") {}
  uint32_t clkHz() const override { return 400000; }
  SpiDataMode dataMode() const override { return mode; }
  SpiBitOrder bitOrder() const override { return kSpiMsbFirst; }
  SpiDataMode mode = kSpiMode0;
};

// Clocks one byte with a stable controller configuration.
uint8_t Exchange(FakeSdSpiCard& card, const Settings& spi,
                 uint8_t byte = 0xff) {
  card.transfer(spi, &byte, 8);
  return byte;
}

// Sends one valid command; the response starts with the following clock byte.
void Command(FakeSdSpiCard& card, const Settings& spi, bool reset) {
  card.cs().write(3.3f);
  card.cs().write(0.0f);
  for (uint8_t byte : std::array<uint8_t, 6>{
           static_cast<uint8_t>(reset ? 0x40 : 0x7a), 0, 0, 0, 0,
           static_cast<uint8_t>(reset ? 0x95 : 0xfd)}) {
    EXPECT_EQ(Exchange(card, spi, byte), 0xff);
  }
}

// Verifies command parsing survives separate transfers, CMD58 returns OCR
// without changing readiness, and CMD0 resets an initialized card.
TEST(SdSpiCard, ResetAndStatus) {
  FakeSdSpiCard card;
  Settings spi;
  Command(card, spi, true);
  EXPECT_EQ(Exchange(card, spi), 0x01);
  EXPECT_EQ(card.resetCommands(), 1u);
  card.setReady(true);
  Command(card, spi, false);
  std::vector<uint8_t> response;
  for (int i = 0; i < 5; ++i) response.push_back(Exchange(card, spi));
  EXPECT_EQ(response, (std::vector<uint8_t>{0, 0xc0, 0xff, 0x80, 0}));
  EXPECT_TRUE(card.ready());
  EXPECT_EQ(card.statusCommands(), 1u);
  Command(card, spi, true);
  EXPECT_EQ(Exchange(card, spi), 1);
  EXPECT_FALSE(card.ready());
  EXPECT_FALSE(card.protocolError());
}

// Verifies response delays, raw OCR injection and removal without a simulated
// filesystem, including deselected transfers returning 0xff.
TEST(SdSpiCard, DelayAndRemoval) {
  FakeSdSpiCard card;
  Settings spi;
  card.setResponseDelay(2);
  card.setOcr(0);
  Command(card, spi, false);
  EXPECT_EQ(Exchange(card, spi), 0xff);
  EXPECT_EQ(Exchange(card, spi), 0xff);
  EXPECT_EQ(Exchange(card, spi), 1);
  for (int i = 0; i < 4; ++i) EXPECT_EQ(Exchange(card, spi), 0);
  card.setPresent(false);
  Command(card, spi, true);
  EXPECT_EQ(Exchange(card, spi), 0xff);
  EXPECT_EQ(card.resetCommands(), 0u);
  card.cs().write(3.3f);
  EXPECT_EQ(Exchange(card, spi), 0xff);
}

// Verifies a bad CRC produces an error response and unsupported SPI modes
// are visible through protocol diagnostics rather than accepting bad traffic.
TEST(SdSpiCard, RejectsInvalidProtocol) {
  FakeSdSpiCard card;
  Settings spi;
  card.cs().write(0.0f);
  for (uint8_t byte : std::array<uint8_t, 6>{0x40, 0, 0, 0, 0, 0}) {
    Exchange(card, spi, byte);
  }
  EXPECT_EQ(Exchange(card, spi), 0x08);
  EXPECT_TRUE(card.protocolError());
  FakeSdSpiCard invalid_mode;
  invalid_mode.cs().write(0.0f);
  spi.mode = kSpiMode3;
  EXPECT_EQ(Exchange(invalid_mode, spi, 0x40), 0xff);
  EXPECT_TRUE(invalid_mode.protocolError());
}
}  // namespace
