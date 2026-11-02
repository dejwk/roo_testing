#include "hardware/i2c.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "glog/logging.h"
#include "roo_testing/frameworks/pico_support/task_support.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"

struct i2c_inst {
  uint index;
};

namespace {
i2c_inst_t instance0{0};
i2c_inst_t instance1{1};

/// Rejects unknown pointers before dereferencing a native controller handle.
uint Index(i2c_inst_t* i2c) {
  CHECK(i2c == &instance0 || i2c == &instance1) << "Invalid Pico I2C instance";
  return i2c->index;
}

/// Maps shared device outcomes to SDK errors without inventing partial counts.
int Result(FakeI2cDevice::Result result, size_t len) {
  switch (result) {
    case FakeI2cDevice::I2C_ERROR_OK:
      return static_cast<int>(len);
    case FakeI2cDevice::I2C_ERROR_TIMEOUT:
      return PICO_ERROR_TIMEOUT;
    case FakeI2cDevice::I2C_ERROR_BUSY:
      return PICO_ERROR_RESOURCE_IN_USE;
    case FakeI2cDevice::I2C_ERROR_MEMORY:
      return PICO_ERROR_INSUFFICIENT_RESOURCES;
    default:
      return PICO_ERROR_GENERIC;
  }
}

/// Preserves STOP intent and stages received bytes until the device succeeds.
int Transfer(i2c_inst_t* i2c, uint8_t address, const uint8_t* src, uint8_t* dst,
             size_t len, bool nostop, uint64_t timeout_us, bool receive) {
  const uint index = Index(i2c);
  roo_testing::pico::ControllerLock controller(2 + index);
  if (address < 8 || address > 0x77 || len == 0 || len > UINT16_MAX ||
      (receive ? dst == nullptr : src == nullptr))
    return PICO_ERROR_INVALID_ARG;
  FakePicoBoard& board = FakePico();
  if (!board.i2cState(index).enabled) return PICO_ERROR_INVALID_STATE;
  if (timeout_us == 0) return PICO_ERROR_TIMEOUT;
  FakeI2cDevice* device = nullptr;
  const int status = board.prepareI2c(index, address, device);
  if (status != PICO_OK) return status;
  const uint16_t millis = std::min<uint64_t>(
      (timeout_us / 1000) + (timeout_us % 1000 != 0 ? 1 : 0), UINT16_MAX);
  std::vector<uint8_t> buffer(receive ? len : 0);
  board.beginI2c(index);
  const FakeI2cDevice::Result result =
      receive ? device->read(buffer.data(), len, !nostop, millis)
              : device->write(src, len, !nostop, millis);
  board.endI2c(index, result == FakeI2cDevice::I2C_ERROR_OK && nostop);
  if (receive && result == FakeI2cDevice::I2C_ERROR_OK) {
    std::copy(buffer.begin(), buffer.end(), dst);
  }
  return Result(result, len);
}

/// Converts an absolute deadline without wrapping negative remaining time.
uint64_t Remaining(absolute_time_t until) {
  const int64_t us = absolute_time_diff_us(get_absolute_time(), until);
  return us > 0 ? static_cast<uint64_t>(us) : 0;
}
}  // namespace

extern "C" {
i2c_inst_t* const i2c0 = &instance0;
i2c_inst_t* const i2c1 = &instance1;

uint i2c_get_index(i2c_inst_t* i2c) { return Index(i2c); }

i2c_inst_t* i2c_get_instance(uint num) {
  CHECK_LT(num, 2u) << "Invalid Pico I2C index";
  return num == 0 ? i2c0 : i2c1;
}

uint i2c_init(i2c_inst_t* i2c, uint baudrate) {
  roo_testing::pico::ControllerLock controller(2 + Index(i2c));
  return FakePico().initI2c(Index(i2c), baudrate);
}

void i2c_deinit(i2c_inst_t* i2c) {
  roo_testing::pico::ControllerLock controller(2 + Index(i2c));
  FakePico().deinitI2c(Index(i2c));
}

uint i2c_set_baudrate(i2c_inst_t* i2c, uint baudrate) {
  roo_testing::pico::ControllerLock controller(2 + Index(i2c));
  return FakePico().setI2cBaudrate(Index(i2c), baudrate);
}

void i2c_set_slave_mode(i2c_inst_t* i2c, bool slave, uint8_t) {
  CHECK(FakePico().i2cState(Index(i2c)).enabled) << "Pico I2C is disabled";
  CHECK(!slave) << "Unimplemented: Pico I2C slave mode";
}

int i2c_write_blocking(i2c_inst_t* i2c, uint8_t address, const uint8_t* src,
                       size_t len, bool nostop) {
  return Transfer(i2c, address, src, nullptr, len, nostop,
                  uint64_t{UINT16_MAX} * 1000, false);
}

int i2c_read_blocking(i2c_inst_t* i2c, uint8_t address, uint8_t* dst,
                      size_t len, bool nostop) {
  return Transfer(i2c, address, nullptr, dst, len, nostop,
                  uint64_t{UINT16_MAX} * 1000, true);
}

int i2c_write_timeout_us(i2c_inst_t* i2c, uint8_t address, const uint8_t* src,
                         size_t len, bool nostop, uint timeout_us) {
  return Transfer(i2c, address, src, nullptr, len, nostop, timeout_us, false);
}

int i2c_read_timeout_us(i2c_inst_t* i2c, uint8_t address, uint8_t* dst,
                        size_t len, bool nostop, uint timeout_us) {
  return Transfer(i2c, address, nullptr, dst, len, nostop, timeout_us, true);
}

int i2c_write_blocking_until(i2c_inst_t* i2c, uint8_t address,
                             const uint8_t* src, size_t len, bool nostop,
                             absolute_time_t until) {
  return Transfer(i2c, address, src, nullptr, len, nostop, Remaining(until),
                  false);
}

int i2c_read_blocking_until(i2c_inst_t* i2c, uint8_t address, uint8_t* dst,
                            size_t len, bool nostop, absolute_time_t until) {
  return Transfer(i2c, address, nullptr, dst, len, nostop, Remaining(until),
                  true);
}

int i2c_write_timeout_per_char_us(i2c_inst_t* i2c, uint8_t address,
                                  const uint8_t* src, size_t len, bool nostop,
                                  uint timeout_us) {
  const uint64_t budget =
      uint64_t{timeout_us} * std::min<size_t>(len, UINT16_MAX);
  return Transfer(i2c, address, src, nullptr, len, nostop, budget, false);
}

int i2c_read_timeout_per_char_us(i2c_inst_t* i2c, uint8_t address, uint8_t* dst,
                                 size_t len, bool nostop, uint timeout_us) {
  const uint64_t budget =
      uint64_t{timeout_us} * std::min<size_t>(len, UINT16_MAX);
  return Transfer(i2c, address, nullptr, dst, len, nostop, budget, true);
}
}
