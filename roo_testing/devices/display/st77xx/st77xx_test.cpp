#include "roo_testing/devices/display/st77xx/st77xx.h"

#include <utility>

#include "gtest/gtest.h"

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
                uint32_t color) override {
    EXPECT_GE(x0, 0);
    EXPECT_GE(y0, 0);
    EXPECT_LT(x1, width());
    EXPECT_LT(y1, height());
    last_x = x0;
    last_y = y0;
    last_color = color;
    ++writes;
  }

  void drawRect(int16_t, int16_t, int16_t, int16_t, const uint32_t*) override {}

  int last_x = -1;
  int last_y = -1;
  uint32_t last_color = 0;
  int writes = 0;
};

class St77xxAddressTest : public testing::TestWithParam<int> {
 protected:
  void sendCommand(uint8_t value) {
    display_.dc().write(0.0f);
    display_.transfer(spi_, &value, 8);
  }

  void sendData(uint8_t value) {
    display_.dc().write(3.3f);
    display_.transfer(spi_, &value, 8);
  }

  // Addresses a native RAM pixel through the selected MADCTL transform.
  void writeNativePixel(int x, int y) {
    if ((GetParam() & 0x40) != 0) x = ram_width_ - 1 - x;
    if ((GetParam() & 0x80) != 0) y = ram_height_ - 1 - y;
    if ((GetParam() & 0x20) != 0) std::swap(x, y);
    sendCommand(0x2A);
    sendData(x >> 8);
    sendData(x & 0xFF);
    sendData(x >> 8);
    sendData(x & 0xFF);
    sendCommand(0x2B);
    sendData(y >> 8);
    sendData(y & 0xFF);
    sendData(y >> 8);
    sendData(y & 0xFF);
    sendCommand(0x2C);
    sendData(0xF8);
    sendData(0x00);
  }

  void SetUp() override {
    display_.rst().write(3.3f);
    sendCommand(0x36);
    sendData(GetParam());
  }

  static constexpr int ram_width_ = 132;
  static constexpr int ram_height_ = 162;
  TestSpi spi_;
  RecordingViewport viewport_;
  FakeSt77xxSpi display_{viewport_, 128, 160, "ST7735", 2, 1};
};

// Verifies that every MADCTL rotation/reflection maps both visible corners
// inside the viewport, including the last pixel rather than width/height.
TEST_P(St77xxAddressTest, MapsVisibleCornersThroughPaddedRam) {
  writeNativePixel(2, 1);
  EXPECT_EQ(viewport_.last_x, 0);
  EXPECT_EQ(viewport_.last_y, 0);
  EXPECT_EQ(viewport_.last_color, 0xFFFF0000u);
  writeNativePixel(129, 160);
  EXPECT_EQ(viewport_.last_x, 127);
  EXPECT_EQ(viewport_.last_y, 159);
  EXPECT_EQ(viewport_.writes, 2);
}

// Verifies that hidden controller-RAM borders never reach the strict viewport.
TEST_P(St77xxAddressTest, DiscardsHiddenBorderPixels) {
  writeNativePixel(0, 1);
  writeNativePixel(131, 1);
  writeNativePixel(2, 0);
  writeNativePixel(2, 161);
  EXPECT_EQ(viewport_.writes, 0);
}

INSTANTIATE_TEST_SUITE_P(Madctl, St77xxAddressTest,
                         testing::Values(0x00, 0x20, 0x40, 0x60, 0x80, 0xA0,
                                         0xC0, 0xE0));

}  // namespace
