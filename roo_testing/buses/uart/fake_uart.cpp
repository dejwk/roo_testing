#include "fake_uart.h"

#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <deque>
#include <limits>
#include <mutex>

#include "glog/logging.h"
#include "roo_testing/host/scheduler_safe_host_lock.h"

ConsoleUartDevice::ConsoleUartDevice(int input_fd, int output_fd)
    : input_fd_(input_fd), output_fd_(output_fd) {}

size_t ConsoleUartDevice::write(const uint8_t* buf, uint16_t size) {
  if (size == 0) return 0;
  CHECK(buf != nullptr) << "Invalid console UART write buffer";
  size_t sent = 0;
  // Direct writes bypass stdio buffering: a standalone Serial.println() must
  // appear even when no later communication occurs. Complete short writes so
  // ESP32 callers retain their existing full-buffer output contract.
  while (sent < size) {
    const ssize_t count = ::write(output_fd_, buf + sent, size - sent);
    if (count < 0 && errno == EINTR) continue;
    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      pollfd descriptor{output_fd_, POLLOUT, 0};
      int ready;
      do {
        ready = poll(&descriptor, 1, -1);
      } while (ready < 0 && errno == EINTR);
      PCHECK(ready > 0) << "Console UART output poll failed";
      CHECK_EQ(descriptor.revents & (POLLERR | POLLNVAL | POLLHUP), 0)
          << "Console UART output descriptor failed";
      continue;
    }
    PCHECK(count > 0) << "Console UART write failed";
    sent += count;
  }
  return sent;
}

size_t ConsoleUartDevice::read(uint8_t* buf, uint16_t size) {
  if (size == 0) return 0;
  CHECK(buf != nullptr) << "Invalid console UART read buffer";
  roo_testing::SchedulerSafeHostLock lock(input_mutex_);
  // A bounded batch keeps signal-masked input access short on FreeRTOS hosts.
  const size_t limit = std::min<size_t>(size, 256);
  size_t count = 0;
  while (count < limit && pollInput()) {
    buf[count++] = *pending_;
    pending_.reset();
  }
  return count;
}

bool ConsoleUartDevice::pollInput() {
  if (pending_.has_value()) return true;
  if (eof_) return false;
  pollfd descriptor{input_fd_, POLLIN, 0};
  int ready;
  do {
    ready = poll(&descriptor, 1, 0);
  } while (ready < 0 && errno == EINTR);
  PCHECK(ready >= 0) << "Console UART input poll failed";
  if (ready == 0) return false;
  CHECK_EQ(descriptor.revents & (POLLERR | POLLNVAL), 0)
      << "Console UART input descriptor failed";
  if ((descriptor.revents & (POLLIN | POLLHUP)) == 0) return false;
  uint8_t byte;
  ssize_t received;
  do {
    received = ::read(input_fd_, &byte, 1);
  } while (received < 0 && errno == EINTR);
  if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return false;
  PCHECK(received >= 0) << "Console UART read failed";
  if (received == 0) {
    eof_ = true;
    return false;
  }
  pending_ = byte;
  return true;
}

size_t ConsoleUartDevice::availableForRead() {
  roo_testing::SchedulerSafeHostLock lock(input_mutex_);
  return pollInput() ? 1 : 0;
}

bool ConsoleUartDevice::inputClosed() const {
  roo_testing::SchedulerSafeHostLock lock(input_mutex_);
  return eof_;
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

namespace roo_testing {
ConsoleUartDevice& HostConsoleUart() {
  static ConsoleUartDevice console;
  return console;
}
}  // namespace roo_testing
