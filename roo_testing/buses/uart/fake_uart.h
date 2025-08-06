#pragma once

#include <stdint.h>

#include <memory>
#include <string>

class FakeUartDevice {
 public:
  FakeUartDevice() {}
  virtual ~FakeUartDevice() {}

  virtual size_t write(const uint8_t* buf, uint16_t size) = 0;
  virtual size_t read(uint8_t* buf, uint16_t size) = 0;
  virtual size_t availableForRead() = 0;
  virtual size_t availableForWrite() { return 1; }
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
  size_t write(const uint8_t* buf, uint16_t size) override;
  size_t read(uint8_t* buf, uint16_t size) override;
  size_t availableForRead() override;
};
