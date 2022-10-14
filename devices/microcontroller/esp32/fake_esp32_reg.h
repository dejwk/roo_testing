#pragma once

#include <cstring>
#include <inttypes.h>

class EspReg {
 public:
  EspReg() {
    memset(buf_, 0, 1024*1024);
  }

  // Sets the val bits with corresponding mask = 1 in the register at
  // the specified address. The addr must be aligned on 4-byte boundary.
  void write(uint32_t addr, uint32_t mask, uint32_t val);

  // Retrieves the bits with corresponding mask = 1 from the register at
  // the specified address. The addr must be aligned on 4-byte boundary.
  // The masked bits are zero.
  uint32_t read(uint32_t addr, uint32_t mask);

 private:
  uint32_t& reg(uint32_t idx);

  // Maps the address range 0x3ff00000-0x3fffffff.
  char buf_[1024*1024];
};