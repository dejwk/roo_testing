#include "soc/soc.h"

#include "glog/logging.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"

extern "C" {

#define HANDLE_SPI_REG_READ_CASE(idx, name, reg) \
  case name(idx): {                              \
    return SPI##idx.reg;                         \
  }

#define HANDLE_SPI_REG_READ(name, reg)   \
  HANDLE_SPI_REG_READ_CASE(0, name, reg) \
  HANDLE_SPI_REG_READ_CASE(1, name, reg) \
  HANDLE_SPI_REG_READ_CASE(2, name, reg) \
  HANDLE_SPI_REG_READ_CASE(3, name, reg)

uint32_t __readPeriReg(uint32_t reg) {
  switch (reg) { HANDLE_SPI_REG_READ(SPI_CMD_REG, cmd.usr); }
  LOG(FATAL) << "Direct reading is not currently supported for the register 0x"
             << std::hex << reg;
}

#define HANDLE_SPI_REG_WRITE_CASE(idx, name, reg, val) \
  case name(idx): {                                    \
    SPI##idx.reg = val;                                \
    return;                                            \
  }

#define HANDLE_SPI_REG_WRITE(name, reg, val)   \
  HANDLE_SPI_REG_WRITE_CASE(0, name, reg, val) \
  HANDLE_SPI_REG_WRITE_CASE(1, name, reg, val) \
  HANDLE_SPI_REG_WRITE_CASE(2, name, reg, val) \
  HANDLE_SPI_REG_WRITE_CASE(3, name, reg, val)

void __writePeriReg(uint32_t reg, uint32_t val) {
  switch (reg) {
    HANDLE_SPI_REG_WRITE(SPI_MOSI_DLEN_REG, mosi_dlen.val, val);
    HANDLE_SPI_REG_WRITE(SPI_W0_REG, data_buf[0], val);
    HANDLE_SPI_REG_WRITE(SPI_W1_REG, data_buf[1], val);
    HANDLE_SPI_REG_WRITE(SPI_W2_REG, data_buf[2], val);
    HANDLE_SPI_REG_WRITE(SPI_W3_REG, data_buf[3], val);
    HANDLE_SPI_REG_WRITE(SPI_W4_REG, data_buf[4], val);
    HANDLE_SPI_REG_WRITE(SPI_W5_REG, data_buf[5], val);
    HANDLE_SPI_REG_WRITE(SPI_W6_REG, data_buf[6], val);
    HANDLE_SPI_REG_WRITE(SPI_W7_REG, data_buf[7], val);
    HANDLE_SPI_REG_WRITE(SPI_W8_REG, data_buf[8], val);
    HANDLE_SPI_REG_WRITE(SPI_W9_REG, data_buf[9], val);
    HANDLE_SPI_REG_WRITE(SPI_W10_REG, data_buf[10], val);
    HANDLE_SPI_REG_WRITE(SPI_W11_REG, data_buf[11], val);
    HANDLE_SPI_REG_WRITE(SPI_W12_REG, data_buf[12], val);
    HANDLE_SPI_REG_WRITE(SPI_W13_REG, data_buf[13], val);
    HANDLE_SPI_REG_WRITE(SPI_W14_REG, data_buf[14], val);
    HANDLE_SPI_REG_WRITE(SPI_W15_REG, data_buf[15], val);
    HANDLE_SPI_REG_WRITE(SPI_CMD_REG, cmd.usr, val);
  }
  LOG(FATAL) << "Direct writing is not currently supported for the register 0x"
             << std::hex << reg;
}
}
