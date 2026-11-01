#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "roo_testing/buses/uart/fake_uart.h"

namespace {

/// Owns private descriptors so tests never consume the process console.
class Pipe {
 public:
  /// Opens a private pipe, reporting setup failures as test failures.
  Pipe() { EXPECT_EQ(pipe(fds_.data()), 0); }

  /// Releases owned endpoints without involving UART descriptor ownership.
  ~Pipe() {
    if (fds_[0] >= 0) close(fds_[0]);
    if (fds_[1] >= 0) close(fds_[1]);
  }

  /// Returns the borrowed input endpoint.
  int reader() const { return fds_[0]; }

  /// Returns the borrowed output endpoint.
  int writer() const { return fds_[1]; }

  /// Closes the writer to deliver EOF after queued bytes have been read.
  void closeWriter() {
    ASSERT_EQ(close(fds_[1]), 0);
    fds_[1] = -1;
  }

 private:
  std::array<int, 2> fds_ = {-1, -1};
};

// Verifies empty open input is nonblocking, lookahead is stable, and short
// reads preserve the unread destination bytes and descriptor flags.
TEST(ConsoleUartTest, PollsInputWithoutPaddingOrChangingFlags) {
  Pipe input;
  ConsoleUartDevice console(input.reader());
  const int flags = fcntl(input.reader(), F_GETFL);
  EXPECT_EQ(console.availableForRead(), 0u);
  EXPECT_FALSE(console.inputClosed());
  std::array<uint8_t, 8> buffer;
  buffer.fill(0xee);
  EXPECT_EQ(console.read(buffer.data(), buffer.size()), 0u);
  const uint8_t data[] = {0, 255, '\n'};
  ASSERT_EQ(write(input.writer(), data, sizeof(data)), 3);
  EXPECT_EQ(console.availableForRead(), 1u);
  EXPECT_EQ(console.availableForRead(), 1u);
  EXPECT_EQ(console.read(nullptr, 0), 0u);
  EXPECT_EQ(console.read(buffer.data(), buffer.size()), 3u);
  EXPECT_EQ(buffer[0], 0);
  EXPECT_EQ(buffer[1], 255);
  EXPECT_EQ(buffer[2], '\n');
  EXPECT_TRUE(std::all_of(buffer.begin() + 3, buffer.end(),
                          [](uint8_t byte) { return byte == 0xee; }));
  EXPECT_EQ(console.availableForRead(), 0u);
  EXPECT_EQ(fcntl(input.reader(), F_GETFL), flags);
}

// Verifies EOF preserves queued/lookahead bytes, then returns zero without
// imposing a platform-specific blocking-read failure policy.
TEST(ConsoleUartTest, ReportsInputClosureAfterQueuedBytes) {
  Pipe input;
  ConsoleUartDevice console(input.reader());
  const uint8_t data = 42;
  ASSERT_EQ(write(input.writer(), &data, 1), 1);
  input.closeWriter();
  EXPECT_EQ(console.availableForRead(), 1u);
  EXPECT_FALSE(console.inputClosed());
  uint8_t byte = 0;
  EXPECT_EQ(console.read(&byte, 1), 1u);
  EXPECT_EQ(byte, 42);
  EXPECT_EQ(console.availableForRead(), 0u);
  EXPECT_TRUE(console.inputClosed());
  byte = 99;
  EXPECT_EQ(console.read(&byte, 1), 0u);
  EXPECT_EQ(byte, 99);
  EXPECT_EQ(console.availableForRead(), 0u);
}

// Verifies output is complete even when a nonblocking pipe forces partial
// writes/EAGAIN. No later call or stdio flush is needed to deliver the bytes.
TEST(ConsoleUartTest, CompletesDescriptorWritesUnderBackpressure) {
  Pipe input;
  Pipe output;
  ConsoleUartDevice console(input.reader(), output.writer());
  const int flags = fcntl(output.writer(), F_GETFL) | O_NONBLOCK;
  ASSERT_EQ(fcntl(output.writer(), F_SETFL, flags), 0);
  std::array<uint8_t, UINT16_MAX> data;
  for (size_t i = 0; i < data.size(); ++i) data[i] = i % 256;
  std::array<uint8_t, UINT16_MAX> received{};
  size_t total = 0;
  std::thread receiver([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    while (total < received.size()) {
      pollfd descriptor{output.reader(), POLLIN, 0};
      if (poll(&descriptor, 1, 10000) <= 0) break;
      const ssize_t count = ::read(output.reader(), received.data() + total,
                                   received.size() - total);
      if (count <= 0) break;
      total += count;
    }
  });
  const size_t count = console.write(data.data(), data.size());
  receiver.join();
  EXPECT_EQ(count, data.size());
  EXPECT_EQ(total, data.size());
  EXPECT_EQ(received, data);
  EXPECT_EQ(console.write(nullptr, 0), 0u);
  EXPECT_EQ(console.availableForWrite(), 1u);
  EXPECT_EQ(fcntl(output.writer(), F_GETFL), flags);
}

// Verifies concurrent native readers serialize lookahead/EOF state and deliver
// each queued byte once, including when the caller runs in the FreeRTOS runner.
TEST(ConsoleUartTest, SerializesConcurrentReaders) {
  Pipe input;
  ConsoleUartDevice console(input.reader());
  std::array<uint8_t, 2048> data;
  for (size_t i = 0; i < data.size(); ++i) data[i] = i % 256;
  ASSERT_EQ(write(input.writer(), data.data(), data.size()), 2048);
  input.closeWriter();
  std::array<std::atomic<uint>, 256> counts{};
  const auto consume = [&console, &counts] {
    uint8_t byte;
    while (!console.inputClosed()) {
      if (console.read(&byte, 1) == 1) ++counts[byte];
    }
  };
  std::thread first(consume);
  std::thread second(consume);
  first.join();
  second.join();
  for (const std::atomic<uint>& count : counts) EXPECT_EQ(count.load(), 8u);
}

}  // namespace
