#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#include "roo_testing/microcontrollers/esp32/soc_traits.h"

// Verifies the board's default console wiring against private subprocess pipes.
int main(int argc, char** argv) {
  if (argc < 2) return 1;
  FakeEsp32().in_matrix.assign(roo_testing::soc::kConsoleRx,
                               roo_testing::soc::kUartRxSignals[0], false);
  Esp32UartInterface& uart = FakeEsp32().uart(0);
  if (strcmp(argv[1], "empty") == 0) {
    return uart.availableForRead() == 0 ? 0 : 2;
  }
  if (strcmp(argv[1], "eof") == 0) {
    uint8_t byte = 42;
    return uart.read(&byte, 1) == 0 && byte == 42 ? 0 : 3;
  }
  if (strcmp(argv[1], "flush") == 0) {
    const uint8_t text[] = "ready\n";
    uart.write(text, sizeof(text) - 1);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    uint8_t byte;
    while (uart.read(&byte, 1) == 0) {
      if (std::chrono::steady_clock::now() >= deadline) return 8;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return byte == '!' ? 0 : 9;
  }
  if (strcmp(argv[1], "text") == 0) {
    const uint8_t text[] = "hello\r\nworld\r\n";
    return uart.write(text, sizeof(text) - 1) == sizeof(text) - 1 ? 0 : 4;
  }
  if (strcmp(argv[1], "echo") != 0 || argc != 3) return 5;
  size_t remaining = std::strtoul(argv[2], nullptr, 10);
  std::array<uint8_t, 1024> bytes;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (remaining != 0) {
    const size_t chunk = remaining < bytes.size() ? remaining : bytes.size();
    const size_t count = uart.read(bytes.data(), chunk);
    if (count == 0) {
      if (std::chrono::steady_clock::now() >= deadline) return 6;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    size_t sent = 0;
    while (sent < count) {
      const size_t written = uart.write(bytes.data() + sent, count - sent);
      if (written == 0) return 7;
      sent += written;
    }
    remaining -= count;
  }
  return 0;
}
