#pragma once

#include <stdint.h>

#include <memory>
#include <string>

class FakeUartDevice {
 public:
  enum Result {
    UART_ERROR_OK = 0,
    UART_ERROR_DEV,
  };

  FakeUartDevice() {}
  virtual ~FakeUartDevice() {}

  virtual Result write(const uint8_t* buf, uint16_t size) = 0;
  virtual Result read(uint8_t* buf, uint16_t size) = 0;
};

class FakeUartInterface {
 public:
  FakeUartInterface(const std::string& name) : name_(name) {}

  ~FakeUartInterface();

  const std::string& name() const { return name_; }

  void attach(FakeUartDevice& device) { attachInternal(&device, false); }

  void attach(std::unique_ptr<FakeUartDevice> device) {
    attachInternal(device.release(), true);
  }

  FakeUartDevice* getDevice() const;

 private:
  struct Attachment {
    Attachment() : ptr(nullptr), owned(false) {}
    Attachment(FakeUartDevice* ptr, bool owned) : ptr(ptr), owned(owned) {}

    FakeUartDevice* ptr;
    bool owned;
  };

  void attachInternal(FakeUartDevice* ptr, bool owned);

  std::string name_;
  Attachment device_;
};

class ConsoleUartDevice : public FakeUartDevice {
 public:
  Result write(const uint8_t* buf, uint16_t size) override;
  Result read(uint8_t* buf, uint16_t size) override;
};
