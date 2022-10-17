#include "fake_esp32_i2c.h"

#include "fake_esp32.h"
#include "glog/logging.h"

FakeI2cDevice* Esp32I2c::resolveOut(uint16_t address) {
  FakeI2cDevice* result = nullptr;
  for (const auto& i : board_.i2c_devices()) {
    if (board_.out_matrix.signal_for_pin(i.second.scl) != scl_signal_) continue;
    if (board_.out_matrix.signal_for_pin(i.second.sda) != sda_signal_) continue;
    FakeI2cDevice* dev = i.first;
    if (dev->address() != address) continue;
    if (result != nullptr) {
      LOG(ERROR) << "I2C interface " << name() << ": bus conflict: selected "
                 << result->name() << " and " << dev->name() << "\n";
      return nullptr;
    }
    result = dev;
  }
  return result;
}

Esp32I2c::Esp32I2c(FakeEsp32Board& board, const std::string& name,
                   uint8_t sda_signal, uint8_t scl_signal)
    : board_(board),
      name_(name),
      sda_signal_(sda_signal),
      scl_signal_(scl_signal),
      txfifo_idx_(0) {}

void Esp32I2c::writeTxFifo(uint8_t data) {
  DCHECK(txfifo_idx_ < 32);
  ram_[txfifo_idx_++] = data;
}

void Esp32I2c::start() {}

void Esp32I2c::write() {}

void Esp32I2c::read() {}
void Esp32I2c::stop() {}
void Esp32I2c::end() {}

int32_t Esp32I2c::write(uint16_t address, const uint8_t* buf, size_t size,
                        uint32_t timeOutMillis) {
  FakeI2cDevice* dev = resolveOut(address);
  if (dev == nullptr) {
    return ESP_ERR_NOT_FOUND;
  }
  return dev->write(buf, size, true, timeOutMillis);
}

int32_t Esp32I2c::read(uint16_t address, uint8_t* buf, size_t size,
                        uint32_t timeOutMillis) {
  FakeI2cDevice* dev = resolveOut(address);
  if (dev == nullptr) {
    return ESP_ERR_NOT_FOUND;
  }
  return dev->read(buf, size, true, timeOutMillis);
}
