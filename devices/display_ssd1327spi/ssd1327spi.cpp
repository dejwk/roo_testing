#include "roo_testing/devices/display_ssd1327spi/ssd1327spi.h"
#include <iostream>
void FakeSsd1327Spi::transfer(uint32_t clk, SpiDataMode mode, SpiBitOrder order,
                              uint8_t* buf, uint16_t bit_count) {
  std::cerr << "Transmitting " << bit_count << " bits";                                  
}
