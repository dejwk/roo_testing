#include "fake_uart.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <limits>
#include <mutex>

size_t ConsoleUartDevice::write(const uint8_t* buf, uint16_t size) {
  std::cout.write((const char*)buf, size);
  // Emulate raw, unbuffered UART output.
  std::cout.flush();
  return size;
}

size_t ConsoleUartDevice::read(uint8_t* buf, uint16_t size) {
  std::istream& is = std::cin;
  size_t total = 0;
  while (size > 0) {
    int read = is.readsome((char*)buf, size);
    if (read <= 0) {
      while (size-- > 0) *buf++ = 0;
      return total;
    }
    buf += read;
    size -= read;
    total += read;
  }
  return total;
}

size_t ConsoleUartDevice::availableForRead() {
  return (std::cin.peek() == std::cin.eof()) ? 0 : 1;
}

class FakeUartCable::Endpoint : public FakeUartDevice {
 public:
  Endpoint() : peer_(nullptr) {}

  void connect(Endpoint& peer) { peer_ = &peer; }

  size_t write(const uint8_t* buf, uint16_t size) override {
    return peer_ == nullptr ? 0 : peer_->receive(buf, size);
  }

  size_t read(uint8_t* buf, uint16_t size) override {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = std::min<size_t>(size, received_.size());
    for (size_t i = 0; i < count; ++i) {
      buf[i] = received_.front();
      received_.pop_front();
    }
    return count;
  }

  size_t availableForRead() override {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_.size();
  }

  size_t availableForWrite() override {
    return std::numeric_limits<uint16_t>::max();
  }

 private:
  size_t receive(const uint8_t* buf, uint16_t size) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      received_.insert(received_.end(), buf, buf + size);
    }
    notifyDataAvailable();
    return size;
  }

  Endpoint* peer_;
  std::mutex mutex_;
  std::deque<uint8_t> received_;
};

FakeUartCable::FakeUartCable()
    : end_a_(std::make_unique<Endpoint>()),
      end_b_(std::make_unique<Endpoint>()) {
  end_a_->connect(*end_b_);
  end_b_->connect(*end_a_);
}

FakeUartCable::~FakeUartCable() = default;

FakeUartDevice& FakeUartCable::end_a() { return *end_a_; }

FakeUartDevice& FakeUartCable::end_b() { return *end_b_; }
