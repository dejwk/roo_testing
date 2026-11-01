#include <array>
#include <cstdio>
#include <vector>

#include "examples/pico_spi/pattern.h"
#include "glog/logging.h"
#include "hardware/gpio.h"
#include "roo_testing/devices/display/st77xx/st77xx.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

namespace {

/// Records final pixels without any display server or GUI toolkit.
class Framebuffer : public roo_testing_transducers::Viewport {
 public:
  /// Applies bounded inclusive rectangles to a native 16x16 pixel array.
  void fillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                uint32_t color) override {
    CHECK(x0 >= 0 && y0 >= 0 && x1 < 16 && y1 < 16);
    for (int y = y0; y <= y1; ++y) {
      for (int x = x0; x <= x1; ++x) pixels[y * 16 + x] = color;
    }
  }

  /// Copies a rectangle supplied by the shared viewport interface.
  void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                const uint32_t* colors) override {
    for (int y = y0; y <= y1; ++y) {
      for (int x = x0; x <= x1; ++x) fillRect(x, y, x, y, *colors++);
    }
  }

  std::array<uint32_t, 256> pixels{};
};

/// Captures transactions while delegating pixel interpretation to the existing
/// ST77xx model; the capture is not a substitute display implementation.
class Display : public FakeSt77xxSpi {
 public:
  /// Creates a 16x16 panel backed by the headless fixture's viewport.
  explicit Display(Framebuffer& viewport) : FakeSt77xxSpi(viewport, 16, 16) {}

  /// Records commands, data, and preserved CS at each adapter chunk boundary.
  void transfer(const FakeSpiInterface& spi, uint8_t* bytes,
                uint16_t bits) override {
    CHECK(isSelected());
    CHECK(!gpio_get(17));
    CHECK_EQ(spi.bitOrder(), kSpiMsbFirst);
    CHECK_EQ(spi.dataMode(), kSpiMode0);
    ++chunks;
    if (gpio_get(20)) {
      data.insert(data.end(), bytes, bytes + bits / 8);
    } else {
      commands.insert(commands.end(), bytes, bytes + bits / 8);
    }
    FakeSt77xxSpi::transfer(spi, bytes, bits);
  }

  std::vector<uint8_t> commands;
  std::vector<uint8_t> data;
  uint chunks = 0;
};

}  // namespace

int main() {
  Framebuffer viewport;
  Display display(viewport);
  FakePico().attachSpiDevice(display, 18, -1, 19);
  FakePico().attachOutput(17, display.cs());
  FakePico().attachOutput(20, display.dc());
  FakePico().attachOutput(21, display.rst());
  CHECK(DrawPicoSpiPattern());
  CHECK(display.commands == std::vector<uint8_t>({1, 0x3a, 0x2a, 0x2b, 0x2c}));
  CHECK_EQ(display.chunks, 10u);
  CHECK_EQ(display.data.size(), 521u);
  CHECK(gpio_get(17));
  for (size_t i = 0; i < viewport.pixels.size(); ++i) {
    const bool red = i % 16 < 8;
    CHECK_EQ(viewport.pixels[i], red ? 0xffff0000u : 0xff00ff00u);
    CHECK_EQ(display.data[9 + 2 * i], red ? 0xf8 : 0x07);
    CHECK_EQ(display.data[10 + 2 * i], red ? 0x00 : 0xe0);
  }
  std::puts("Pico SPI ST77xx: 16x16 red/green frame, 512 pixel bytes verified");
  return 0;
}
