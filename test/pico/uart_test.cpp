#include "hardware/uart.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"
#include "roo_testing/transducers/voltage/voltage.h"

namespace {

/// Forces short/no-progress transfers while retaining the shared cable model.
class ChunkedDevice : public FakeUartDevice {
 public:
  /// Borrows a cable endpoint for the fixture lifetime.
  explicit ChunkedDevice(FakeUartDevice& device) : device_(device) {}

  /// Sends at most limit bytes and records the largest requested chunk.
  size_t write(const uint8_t* bytes, uint16_t size) override {
    largest_write = std::max(largest_write, size);
    if (stall_write) {
      stall_write = false;
      return 0;
    }
    return device_.write(bytes, std::min(size, limit));
  }

  /// Receives short chunks without filling unread bytes.
  size_t read(uint8_t* bytes, uint16_t size) override {
    largest_read = std::max(largest_read, size);
    if (stall_read) {
      stall_read = false;
      return 0;
    }
    return device_.read(bytes, std::min(size, limit));
  }

  /// Delegates queue readiness without consuming data.
  size_t availableForRead() override { return device_.availableForRead(); }

  /// Delegates device write capacity.
  size_t availableForWrite() override { return device_.availableForWrite(); }

  uint16_t limit = UINT16_MAX;
  uint16_t largest_write = 0;
  uint16_t largest_read = 0;
  bool stall_read = false;
  bool stall_write = false;

 private:
  FakeUartDevice& device_;
};

/// Owns immutable wiring; group 6 remains free for attachment failure checks.
struct Wiring {
  Wiring() {
    for (uint group = 0; group < cables.size(); ++group) {
      devices[group] = std::make_unique<ChunkedDevice>(
          group == 1 ? cables[0].end_b() : cables[group].end_a());
      if (group != 6) {
        FakePico().attachUartDevice(*devices[group], group * 4, group * 4 + 1);
      }
    }
  }

  std::array<FakeUartCable, 8> cables;
  std::array<std::unique_ptr<ChunkedDevice>, 8> devices;
};

Wiring& Connections() {
  static Wiring wiring;
  return wiring;
}

/// Returns the remote cable endpoint of a physical fixture route.
FakeUartDevice& Peer(uint group) {
  return group == 1 ? Connections().cables[0].end_a()
                    : Connections().cables[group].end_b();
}

/// Selects just one complete physical route without changing fixture wiring.
void Select(uint group) {
  for (uint pin = 0; pin < FakePicoBoard::kGpioCount; ++pin) {
    if (pin % 4 < 2) gpio_set_function(pin, GPIO_FUNC_NULL);
  }
  gpio_set_function(group * 4, GPIO_FUNC_UART);
  gpio_set_function(group * 4 + 1, GPIO_FUNC_UART);
}

/// Drains queue state without using the SDK or advancing the shared clock.
void Drain(FakeUartDevice& device) {
  uint8_t scratch[256];
  while (device.read(scratch, sizeof(scratch)) != 0) {
  }
}

class PicoUartTest : public testing::Test {
 protected:
  void SetUp() override {
    Wiring& wiring = Connections();
    for (uint pin = 0; pin < FakePicoBoard::kGpioCount; ++pin) {
      gpio_set_function(pin, GPIO_FUNC_NULL);
    }
    for (uint group = 0; group < wiring.cables.size(); ++group) {
      Drain(wiring.cables[group].end_a());
      Drain(wiring.cables[group].end_b());
      wiring.devices[group]->limit = UINT16_MAX;
      wiring.devices[group]->stall_read = false;
      wiring.devices[group]->stall_write = false;
    }
    uart_init(uart0, 115200);
    uart_init(uart1, 115200);
  }
};

// Verifies stable non-null identities, disabled lifecycle, and divisor
// rounding.
TEST_F(PicoUartTest, InstancesLifecycleAndBaudrate) {
  EXPECT_NE(uart0, nullptr);
  EXPECT_NE(uart1, nullptr);
  EXPECT_NE(uart0, uart1);
  EXPECT_EQ(uart_get_instance(0), uart0);
  EXPECT_EQ(uart_get_instance(1), uart1);
  EXPECT_EQ(uart_get_index(uart1), 1u);
  EXPECT_EQ(uart_init(uart0, 115200), 115207u);
  EXPECT_EQ(uart_set_baudrate(uart0, 1), 119u);
  EXPECT_EQ(uart_set_baudrate(uart0, UINT32_MAX), 7812500u);
  EXPECT_EQ(FakePico().uartState(0).baudrate, 7812500u);
  EXPECT_EQ(FakePico().uartState(1).baudrate, 115207u);
  uart_set_translate_crlf(uart0, true);
  uart_deinit(uart0);
  EXPECT_FALSE(uart_is_enabled(uart0));
  EXPECT_TRUE(uart_is_enabled(uart1));
  EXPECT_DEATH(uart_putc_raw(uart0, 'x'), "disabled");
  EXPECT_DEATH(uart_set_baudrate(uart0, 9600), "disabled");
  uart_init(uart0, 9600);
  EXPECT_FALSE(FakePico().uartState(0).translate_crlf);
  // A zero-byte operation validates lifecycle but needs neither buffer nor
  // pins.
  uart_write_blocking(uart0, nullptr, 0);
  uart_read_blocking(uart0, nullptr, 0);
}

// Verifies every GP0..29 TX/RX group uses its actual RP2040 UART controller.
TEST_F(PicoUartTest, PhysicalPinRoutes) {
  constexpr uint indices[] = {0, 1, 1, 0, 0, 1, 1, 0};
  for (uint group = 0; group < 8; ++group) {
    Select(group);
    uart_inst_t* uart = uart_get_instance(indices[group]);
    EXPECT_DEATH(uart_is_writable(uart_get_instance(1 - indices[group])),
                 "exactly one active pin route");
    if (group == 6) {
      EXPECT_DEATH(uart_is_writable(uart), "no attached device");
      continue;
    }
    FakeUartDevice& peer = Peer(group);
    EXPECT_TRUE(uart_is_writable(uart));
    EXPECT_FALSE(uart_is_readable(uart));
    uart_putc_raw(uart, static_cast<char>(group));
    uint8_t byte = 255;
    ASSERT_EQ(peer.read(&byte, 1), 1u);
    EXPECT_EQ(byte, group);
    byte += 32;
    ASSERT_EQ(peer.write(&byte, 1), 1u);
    EXPECT_TRUE(uart_is_readable(uart));
    EXPECT_EQ(static_cast<uint8_t>(uart_getc(uart)), byte);
  }
}

// Verifies both SDK controllers communicate bidirectionally through fake cable.
TEST_F(PicoUartTest, CableConnectsControllers) {
  Select(0);
  gpio_set_function(4, GPIO_FUNC_UART);
  gpio_set_function(5, GPIO_FUNC_UART);
  uart_puts(uart0, "ping");
  uint8_t bytes[4];
  uart_read_blocking(uart1, bytes, 4);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(bytes), 4), "ping");
  uart_puts(uart1, "pong");
  uart_read_blocking(uart0, bytes, 4);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(bytes), 4), "pong");
}

// Verifies large binary transfers handle short and zero progress without
// padding.
TEST_F(PicoUartTest, PartialTransfersAndChunkBounds) {
  Select(0);
  ChunkedDevice& device = *Connections().devices[0];
  FakeUartDevice& peer = Connections().cables[0].end_b();
  device.limit = 257;
  device.stall_write = true;
  device.stall_read = true;
  std::vector<uint8_t> expected(70000);
  for (size_t i = 0; i < expected.size(); ++i) expected[i] = i;
  const uint64_t before = time_us_64();
  uart_write_blocking(uart0, expected.data(), expected.size());
  std::vector<uint8_t> received(expected.size(), 0xee);
  ASSERT_EQ(peer.read(received.data(), UINT16_MAX), size_t{UINT16_MAX});
  ASSERT_EQ(peer.read(received.data() + UINT16_MAX, 4465), 4465u);
  EXPECT_EQ(received, expected);
  peer.write(expected.data(), size_t{UINT16_MAX});
  peer.write(expected.data() + UINT16_MAX, 4465);
  std::fill(received.begin(), received.end(), 0xee);
  uart_read_blocking(uart0, received.data(), received.size());
  EXPECT_EQ(received, expected);
  EXPECT_EQ(device.largest_read, UINT16_MAX);
  EXPECT_EQ(device.largest_write, UINT16_MAX);
  EXPECT_EQ(time_us_64(), before);
}

// Verifies an empty blocking read waits for a real producer in manual time.
TEST_F(PicoUartTest, BlockingReadWaitsForHostProducer) {
  Select(0);
  FakeUartDevice& peer = Connections().cables[0].end_b();
  std::thread producer([&peer] {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const uint8_t bytes[] = {0, 255};
    peer.write(bytes, 2);
  });
  uint8_t received[] = {11, 22};
  const uint64_t before = time_us_64();
  uart_read_blocking(uart0, received, 2);
  producer.join();
  EXPECT_EQ(received[0], 0);
  EXPECT_EQ(received[1], 255);
  EXPECT_EQ(time_us_64(), before);
}

// Verifies bounded readiness advances manual time and sees shared device
// alarms.
TEST_F(PicoUartTest, ReadableTimeoutUsesSharedClock) {
  Select(0);
  const uint64_t before = time_us_64();
  EXPECT_FALSE(uart_is_readable_within_us(uart0, 0));
  EXPECT_EQ(time_us_64(), before);
  EXPECT_FALSE(uart_is_readable_within_us(uart0, 2301));
  EXPECT_EQ(time_us_64(), before + 2301);
  ScheduleSystemTimeAlarm(time_us_64() + 100, [] {
    const uint8_t byte = 42;
    Connections().cables[0].end_b().write(&byte, 1);
  });
  EXPECT_TRUE(uart_is_readable_within_us(uart0, 200));
  EXPECT_EQ(uart_getc(uart0), 42);
}

// Verifies CRLF text translation, binary bypass, and queue retention on reset.
TEST_F(PicoUartTest, TextTranslationAndDeinitPreserveDeviceQueue) {
  Select(0);
  uart_set_translate_crlf(uart0, true);
  uart_puts(uart0, "a\nb\r\nc");
  uart_putc(uart0, '\n');
  uart_putc_raw(uart0, '\n');
  const uint8_t raw[] = {0, '\n'};
  uart_write_blocking(uart0, raw, 2);
  uart_default_tx_wait_blocking();
  uart_tx_wait_blocking(uart0);
  uart_deinit(uart0);
  uart_init(uart0, 9600);
  uint8_t result[32];
  const std::string expected("a\r\nb\r\nc\r\n\n\0\n", 12);
  const size_t count = Connections().cables[0].end_b().read(result, 32);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(result), count), expected);
}

// Verifies unsupported modes and invalid handles/buffers fail explicitly.
TEST_F(PicoUartTest, UnsupportedCallsAndHandles) {
  Select(0);
  uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
  uart_set_hw_flow(uart0, false, false);
  EXPECT_DEATH(uart_get_index(nullptr), "Invalid Pico UART instance");
  EXPECT_DEATH(uart_init(reinterpret_cast<uart_inst_t*>(123), 9600),
               "instance");
  EXPECT_DEATH(uart_get_instance(2), "index");
  EXPECT_DEATH(uart_set_baudrate(uart0, 0), "baudrate");
  EXPECT_DEATH(uart_set_format(uart0, 7, 1, UART_PARITY_NONE), "8N1");
  EXPECT_DEATH(uart_set_format(uart0, 8, 2, UART_PARITY_NONE), "8N1");
  EXPECT_DEATH(uart_set_format(uart0, 8, 1, UART_PARITY_EVEN), "8N1");
  EXPECT_DEATH(uart_set_hw_flow(uart0, true, false), "flow control");
  EXPECT_DEATH(uart_set_irq_enables(uart0, false, false), "IRQs");
  EXPECT_DEATH(uart_set_irqs_enabled(uart0, true, false), "IRQs");
  EXPECT_DEATH(uart_set_break(uart0, false), "break");
  EXPECT_DEATH(uart_write_blocking(uart0, nullptr, 1), "buffer");
  EXPECT_DEATH(uart_read_blocking(uart0, nullptr, 1), "buffer");
  EXPECT_DEATH(uart_puts(uart0, nullptr), "string");
}

// Verifies mux ambiguity, pad overrides, and voltage observations never fake
// I/O.
TEST_F(PicoUartTest, InvalidRoutesAndPadModes) {
  EXPECT_DEATH(uart_is_readable(uart0), "exactly one active pin route");
  Select(0);
  gpio_set_function(12, GPIO_FUNC_UART);
  EXPECT_DEATH(uart_is_writable(uart0), "exactly one active pin route");
  gpio_set_function(12, GPIO_FUNC_NULL);
  gpio_set_input_enabled(1, false);
  EXPECT_DEATH(uart_is_readable(uart0), "RX pad configuration");
  gpio_set_input_enabled(1, true);
  gpio_set_inover(1, GPIO_OVERRIDE_INVERT);
  EXPECT_DEATH(uart_getc(uart0), "RX pad configuration");
  EXPECT_DEATH(FakePico().padVoltage(0), "UART pin voltage");
  EXPECT_DEATH(FakePico().driveSignal(0), "UART pin voltage");
  EXPECT_DEATH(gpio_set_function(2, GPIO_FUNC_UART),
               "Unsupported Pico GPIO function");
  EXPECT_DEATH(
      {
        gpio_init(0);
        gpio_set_dir(0, GPIO_OUT);
      },
      "SIO output");
}

// Verifies conflicting, invalid, duplicate, and late borrowed wiring is
// rejected.
TEST_F(PicoUartTest, InvalidAttachments) {
  FakeUartCable spare;
  EXPECT_DEATH(FakePico().attachUartDevice(spare.end_a(), -1, -1),
               "needs a TX or RX");
  EXPECT_DEATH(FakePico().attachUartDevice(spare.end_a(), -2, 25), "TX pin");
  EXPECT_DEATH(FakePico().attachUartDevice(spare.end_a(), 30, -1), "GPIO pin");
  EXPECT_DEATH(FakePico().attachUartDevice(spare.end_a(), 0, 1),
               "attachment conflict");
  EXPECT_DEATH(FakePico().attachUartDevice(spare.end_a(), 24, 25),
               "before configuring");
  roo_testing_transducers::SimpleVoltageSink sink("conflict");
  EXPECT_DEATH(FakePico().attachOutput(0, sink), "attachment conflict");
}

}  // namespace
