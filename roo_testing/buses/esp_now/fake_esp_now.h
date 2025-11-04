#pragma once

#include <cstdint>
#include <functional>
#include <utility>

class FakeEspNowDevice {
 public:
  using ReceiveCb = std::function<void(const void*, size_t)>;

  FakeEspNowDevice(uint64_t addr) : addr_(addr) {}

  virtual ~FakeEspNowDevice() {}

  // Send data to the device.
  virtual void send(const void* data, size_t len) = 0;

 protected:
  uint64_t mac_addr() const { return addr_; }

  // Device sending data back over ESP-NOW.
  void respond(const void* data, size_t len) { 
    if (receive_cb_ != nullptr) receive_cb_(data, len);
  }

 private:
  friend class EspNowInterface;

  void set_receive_cb(ReceiveCb receive_cb) {
    receive_cb_ = std::move(receive_cb);
  }

  uint64_t addr_;
  ReceiveCb receive_cb_;
};
