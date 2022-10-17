#include "fake_esp32_reg.h"

#include "fake_esp32.h"
#include "fake_esp32_i2c.h"
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

// Per
// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf,
// the addresses in the range 0x60000000-0x6003FFFF refer to the same content as
// 0x3ff40000-0x3ff7ffff.
constexpr uint32_t remap(uint32_t addr) {
  return (addr >= 0x60000000 && addr <= 0x6003ffff)
             ? addr - (0x60000000 - 0x3ff40000)
             : addr;
}

}  // namespace

EspReg::EspReg(FakeEsp32Board& board) : board_(board) {
  memset(buf_, 0, 1024 * 1024);
}

uint32_t& EspReg::reg(uint32_t addr) {
  DCHECK((addr & 3) == 0) << "Addr " << hex32(addr)
                          << " is not aligned to 4 bytes";
  return ((uint32_t*)buf_)[(remap(addr) - 0x3FF00000) >> 2];
}

void EspReg::write(uint32_t addr, uint32_t mask, uint32_t val) {
  LOG(INFO) << "Reg wr: " << hex32(addr) << ", " << hex32(mask) << " <- "
            << hex32(val);
  addr = remap(addr);
  switch (addr) {
    case remap(0x6001301C): {
      board_.i2c(0).writeTxFifo((uint8_t)val);
      break;
    }
    case remap(0x6002701C): {
      board_.i2c(1).writeTxFifo((uint8_t)val);
      break;
    }

    default:
      break;
  }
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
