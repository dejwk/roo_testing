#include "soc/soc.h"

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

extern "C" {

void fake_esp32_write_reg(uint32_t addr, uint32_t mask, uint32_t val) {
  FakeEsp32().reg().write(addr, mask, val);
}

uint32_t fake_esp32_read_reg(uint32_t addr, uint32_t mask) {
  return FakeEsp32().reg().read(addr, mask);
}

}