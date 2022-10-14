#include "fake_esp32_reg.h"

#include "fake_esp32.h"
#include "glog/logging.h"

namespace {
std::string hex32(uint32_t val) {
  std::string result;
  result.resize(8);
  for (int i = 7; i >= 0; --i) {
    uint8_t d = val & 0xF;
    result[i] = (d >= 10 ? d - 10 + 'A' : d + '0');
    val >>= 4;
  }
  return result;
}
}  // namespace

uint32_t& EspReg::reg(uint32_t addr) {
  DCHECK((addr & 3) == 0) << "Addr " << hex32(addr)
                          << " is not aligned to 4 bytes";
  return ((uint32_t*)buf_)[(addr - 0x3FF00000) >> 2];
}

void EspReg::write(uint32_t addr, uint32_t mask, uint32_t val) {
  LOG(INFO) << "Reg wr: " << hex32(addr) << ", " << hex32(mask) << " <- "
            << hex32(val);
  // Always write-through.
  uint32_t& v = reg(addr);
  v &= ~mask;
  v |= (val & mask);
}

uint32_t EspReg::read(uint32_t addr, uint32_t mask) {
  // Default to read-through.
  uint32_t val = reg(addr) & mask;
  LOG(INFO) << "Reg rd: " << hex32(addr) << ", " << hex32(mask) << " -> "
            << hex32(val);
  return val;
}
