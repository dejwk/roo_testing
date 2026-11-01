#include "examples/pico_spi/pattern.h"

#include <array>

#include "hardware/gpio.h"
#include "hardware/spi.h"

namespace {

/// Sends one display command and its optional parameters under software CS.
bool Register(uint8_t command, const uint8_t* parameters, size_t size) {
  gpio_put(17, false);
  gpio_put(20, false);
  const bool sent_command = spi_write_blocking(spi0, &command, 1) == 1;
  gpio_put(20, true);
  const bool sent_data =
      spi_write_blocking(spi0, parameters, size) == static_cast<int>(size);
  gpio_put(17, true);
  return sent_command && sent_data;
}

}  // namespace

bool DrawPicoSpiPattern() {
  for (uint pin : {17u, 20u, 21u}) {
    gpio_init(pin);
    gpio_put(pin, true);
    gpio_set_dir(pin, GPIO_OUT);
  }
  spi_init(spi0, 8000000);
  gpio_set_function(18, GPIO_FUNC_SPI);
  gpio_set_function(19, GPIO_FUNC_SPI);
  const uint8_t format = 0x55;
  const uint8_t range[] = {0, 0, 0, 15};
  if (!Register(0x01, nullptr, 0) || !Register(0x3a, &format, 1) ||
      !Register(0x2a, range, 4) || !Register(0x2b, range, 4)) {
    return false;
  }
  std::array<uint16_t, 256> pixels;
  for (size_t i = 0; i < pixels.size(); ++i) {
    pixels[i] = i % 16 < 8 ? 0xf800 : 0x07e0;
  }
  gpio_put(17, false);
  gpio_put(20, false);
  const uint8_t command = 0x2c;
  if (spi_write_blocking(spi0, &command, 1) != 1) return false;
  gpio_put(20, true);
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  const bool sent =
      spi_write16_blocking(spi0, pixels.data(), pixels.size()) == 256;
  gpio_put(17, true);
  spi_deinit(spi0);
  return sent;
}
