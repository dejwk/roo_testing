#pragma once

#include <stdint.h>

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
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

  /// Reports permanent input closure after it is observed by read/readiness.
  /// Live devices default to false. Blocking/EOF policy belongs to the caller.
  virtual bool inputClosed() const { return false; }

  void setDataAvailableFn(DataAvailableFn fn) {
    data_available_fn_ = std::move(fn);
  }

  void notifyDataAvailable() {
    if (data_available_fn_ != nullptr) data_available_fn_(this);
  }

 private:
  DataAvailableFn data_available_fn_;
};

/// Bridges UART bytes through borrowed host stdin/stdout file descriptors.
/// Input readiness and reads poll without waiting and never pad unread bytes.
/// Terminal modes and descriptor flags remain unchanged. Use this as the sole
/// reader of the input descriptor; do not mix libc/iostream input with it.
/// Input methods serialize native/FreeRTOS callers with a short host-state
/// lock.
class ConsoleUartDevice : public FakeUartDevice {
 public:
  /// Creates a console on borrowed @p input_fd and @p output_fd.
  /// Neither is closed; both must remain valid through device activity.
  explicit ConsoleUartDevice(int input_fd = 0, int output_fd = 1);

  /// Sends the entire buffer directly to the output descriptor before
  /// returning. Retries short/interrupted writes and waits for nonblocking
  /// backpressure. Bypasses stdio buffering, so output needs no later
  /// communication or flush. Returns the requested size; unrecoverable I/O
  /// errors fail explicitly.
  size_t write(const uint8_t* buf, uint16_t size) override;

  /// Copies currently ready bytes, returning a short count or zero at EOF.
  /// Bounds each batch to keep the host-state lock short; zero-length reads do
  /// nothing. Blocking callers retry and decide how to handle inputClosed().
  size_t read(uint8_t* buf, uint16_t size) override;

  /// Polls readiness with one-byte lookahead; returns zero or one without
  /// waiting.
  size_t availableForRead() override;

  /// Reports observed EOF after the last buffered byte has been consumed.
  bool inputClosed() const override;

 private:
  /// Polls input while holding input_mutex_; retains a byte or records EOF.
  bool pollInput();

  int input_fd_;
  int output_fd_;
  std::optional<uint8_t> pending_;
  bool eof_ = false;
  mutable std::mutex input_mutex_;
};

namespace roo_testing {
/// Returns the lazily constructed shared host console, without terminal
/// changes.
ConsoleUartDevice& HostConsoleUart();
}  // namespace roo_testing

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
