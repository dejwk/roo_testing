#include "roo_testing/devices/display/ili9341/ili9341spi.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "roo_testing/buses/spi/fake_spi.h"
#include "roo_testing/transducers/ui/viewport/viewport.h"

namespace {

class TestSpi : public FakeSpiInterface {
 public:
  TestSpi() : FakeSpiInterface("test") {}

  uint32_t clkHz() const override { return 1000000; }

  SpiDataMode dataMode() const override { return kSpiMode0; }

  SpiBitOrder bitOrder() const override { return kSpiMsbFirst; }
};

class RecordingViewport : public roo_testing_transducers::Viewport {
 public:
  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color_argb) override {
    last_y = y0;
    last_color = color_argb;
  }

  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                const uint32_t* color_argb) override {}

  int16_t last_y = -1;
  uint32_t last_color = 0;
};

class Ili9341PageAddressTest : public testing::TestWithParam<int> {
 protected:
  Ili9341PageAddressTest() : display_(viewport_) { display_.rst().write(3.3f); }

  void command(uint8_t value) {
    display_.dc().write(0.0f);
    display_.transfer(spi_, &value, 8);
  }

  void data(uint8_t value) {
    display_.dc().write(3.3f);
    display_.transfer(spi_, &value, 8);
  }

  TestSpi spi_;
  RecordingViewport viewport_;
  FakeIli9341Spi display_;
};

// Verifies that an interrupted PASET preserves the previous page, and that
// receiving the fourth byte commits the new page across separate SPI transfers.
TEST_P(Ili9341PageAddressTest, CommitsOnlyAfterAllFourBytes) {
  command(0x2A);  // CASET: column 0.
  data(0);
  data(0);
  data(0);
  data(0);
  command(0x2B);  // PASET: establish a complete previous page address.
  data(0);
  data(5);
  data(0);
  data(6);

  command(0x2B);
  const uint8_t page[] = {0x01, 0x02, 0x01, 0x03};  // Rows 258 through 259.
  for (int i = 0; i < GetParam(); ++i) {
    data(page[i]);
  }
  command(0x2C);  // RAMWR exposes the committed page through a pixel write.
  data(0xF8);
  data(0x00);

  EXPECT_EQ(GetParam() == 4 ? 258 : 5, viewport_.last_y);
  EXPECT_EQ(0xFFFF0000u, viewport_.last_color);
}

INSTANTIATE_TEST_SUITE_P(ReceivedBytes, Ili9341PageAddressTest,
                         testing::Range(0, 5));

}  // namespace
