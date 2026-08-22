#pragma once

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>

class FakeUartDevice {
 public:
  using DataAvailableFn = std::function<void(const FakeUartDevice*)>;

  FakeUartDevice() : data_available_fn_(nullptr) {}

  virtual ~FakeUartDevice() {}

  virtual size_t write(const uint8_t* buf, uint16_t size) = 0;
  virtual size_t read(uint8_t* buf, uint16_t size) = 0;
  virtual size_t availableForRead() = 0;
  virtual size_t availableForWrite() { return 1; }

  void setDataAvailableFn(DataAvailableFn fn) {
    data_available_fn_ = std::move(fn);
  }

  void notifyDataAvailable() {
    if (data_available_fn_ != nullptr) data_available_fn_(this);
  }

 private:
  DataAvailableFn data_available_fn_;
};

class ConsoleUartDevice : public FakeUartDevice {
 public:
  size_t write(const uint8_t* buf, uint16_t size) override;
  size_t read(uint8_t* buf, uint16_t size) override;
  size_t availableForRead() override;
};

// A bidirectional cable connecting two UART endpoints.
//
// Bytes written by one endpoint become immediately available for reading from
// the other endpoint. Attach each endpoint to a different UART device to
// emulate crossed TX/RX jumper wires.
class FakeUartCable {
 public:
  FakeUartCable();
  ~FakeUartCable();

  FakeUartCable(const FakeUartCable&) = delete;
  FakeUartCable& operator=(const FakeUartCable&) = delete;

  FakeUartDevice& end_a();
  FakeUartDevice& end_b();

 private:
  class Endpoint;

  std::unique_ptr<Endpoint> end_a_;
  std::unique_ptr<Endpoint> end_b_;
};
