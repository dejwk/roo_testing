#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

class FakeI2cDevice {
 public:
  enum Result {
    I2C_ERROR_OK = 0,
    I2C_ERROR_DEV,
    I2C_ERROR_ACK,
    I2C_ERROR_TIMEOUT,
    I2C_ERROR_BUS,
    I2C_ERROR_BUSY,
    I2C_ERROR_MEMORY,
    I2C_ERROR_CONTINUE,
    I2C_ERROR_NO_BEGIN
  };

  FakeI2cDevice(uint16_t address) : address_(address) {}
  virtual ~FakeI2cDevice() {}

  virtual Result write(uint8_t* buff, uint16_t size, bool sendStop,
                       uint16_t timeOutMillis) = 0;
  virtual Result read(uint8_t* buff, uint16_t size, bool sendStop,
                      uint16_t timeOutMillis) = 0;

  uint16_t address() const { return address_; }

 private:
  uint16_t address_;
};

class FakeI2cInterface {
 public:
  FakeI2cInterface(const std::string& name) : name_(name) {}

  FakeI2cInterface(std::initializer_list<FakeI2cDevice*> devices);

  const std::string& name() const { return name_; }

  void attach(FakeI2cDevice* device) {
    attach(std::unique_ptr<FakeI2cDevice>(device));
  }
  void attach(std::unique_ptr<FakeI2cDevice> device);
  FakeI2cDevice* getDevice(uint16_t address) const;

 private:
  std::string name_;
  std::map<uint16_t, std::unique_ptr<FakeI2cDevice>> devices_;
};

FakeI2cInterface* getI2cInterface(uint8_t i2c_num);
