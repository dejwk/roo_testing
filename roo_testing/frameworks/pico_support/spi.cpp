#include "hardware/spi.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <limits>

#include "glog/logging.h"
#include "roo_testing/frameworks/pico_support/task_support.h"
#include "roo_testing/host/scheduler_signal_guard.h"
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#include "roo_testing/system/timer.h"

struct spi_inst {
  uint index;
};

namespace {

spi_inst_t spi0_instance{0};
spi_inst_t spi1_instance{1};

/// Checks opaque native identity before dereferencing a caller's handle.
uint Index(const spi_inst_t* spi) {
  CHECK(spi == &spi0_instance || spi == &spi1_instance)
      << "Invalid Pico SPI instance";
  return spi->index;
}

/// Gives existing device models an immutable transaction configuration.
class SpiInterface : public FakeSpiInterface {
 public:
  /// Captures the enabled controller's baud, mode, and MSB-first format.
  SpiInterface(uint index, FakePicoBoard::SpiState state)
      : FakeSpiInterface(index == 0 ? "pico_spi0" : "pico_spi1"),
        state_(state) {}

  /// Returns the achieved SCK rate.
  uint32_t clkHz() const override { return state_.baudrate; }

  /// Returns the selected CPOL/CPHA combination.
  SpiDataMode dataMode() const override { return state_.mode; }

  /// Matches the PL022 controller's supported wire order.
  SpiBitOrder bitOrder() const override { return kSpiMsbFirst; }

 private:
  FakePicoBoard::SpiState state_;
};

/// Rounds a cumulative number of serial bits up to whole nanoseconds.
uint64_t DurationNs(uint64_t bits, uint baudrate) {
  return (bits / baudrate) * 1000000000 +
         ((bits % baudrate) * 1000000000 + baudrate - 1) / baudrate;
}

/// Rejects partial source/destination overlap before any transfer effects.
bool Overlaps(const void* src, const void* dst, size_t bytes) {
  if (src == dst) return false;
  const uintptr_t a = reinterpret_cast<uintptr_t>(src);
  const uintptr_t b = reinterpret_cast<uintptr_t>(dst);
  return a < b ? b - a < bytes : a - b < bytes;
}

/// Exchanges bounded whole-frame chunks while preserving the SDK call's CS
/// selection. Timing uses cumulative rounding and the existing SPI clock path.
template <typename Word>
int Transfer(spi_inst_t* spi, const Word* src, Word* dst, size_t len,
             Word repeated, bool send, bool receive) {
  FakePicoBoard& board = FakePico();
  const uint index = Index(spi);
  roo_testing::pico::ControllerLock controller(index);
  const FakePicoBoard::SpiState state = board.spiState(index);
  if (!state.enabled) return PICO_ERROR_INVALID_STATE;
  if (state.data_bits != 8 * sizeof(Word) || len > INT_MAX ||
      (len != 0 && ((send && src == nullptr) || (receive && dst == nullptr)))) {
    return PICO_ERROR_INVALID_ARG;
  }
  if (len == 0) return 0;
  if (send && receive && Overlaps(src, dst, len * sizeof(Word))) {
    return PICO_ERROR_INVALID_ARG;
  }
  const uint64_t duration =
      DurationNs(static_cast<uint64_t>(len) * state.data_bits, state.baudrate);
  constexpr uint64_t limit = std::numeric_limits<int64_t>::max() - 999;
  const uint64_t now_ns =
      static_cast<uint64_t>(system_time_get_micros()) * 1000;
  if (duration > limit || now_ns > limit - duration) {
    return PICO_ERROR_INVALID_ARG;
  }
  FakePicoBoard::SpiRoute route;
  const int status = board.prepareSpi(index, receive, route);
  if (status != PICO_OK) return status;

  SpiInterface interface(index, state);
  board.beginSpi(index);
  constexpr size_t kChunkBytes = 256;
  constexpr size_t kChunkWords = kChunkBytes / sizeof(Word);
  std::array<uint8_t, kChunkBytes> outgoing;
  std::array<uint8_t, kChunkBytes> scratch;
  std::array<uint8_t, kChunkBytes> incoming;
  uint64_t elapsed = 0;
  for (size_t offset = 0; offset < len;) {
    const size_t words = std::min(len - offset, kChunkWords);
    const size_t bytes = words * sizeof(Word);
    for (size_t i = 0; i < words; ++i) {
      const Word value = send ? src[offset + i] : repeated;
      if constexpr (sizeof(Word) == 2) outgoing[2 * i] = value >> 8;
      outgoing[sizeof(Word) * i + sizeof(Word) - 1] = value & 0xff;
    }
    if (receive) incoming.fill(route.idle_high ? 0xff : 0);
    for (SimpleFakeSpiDevice* device : route.devices) {
      std::copy_n(outgoing.begin(), bytes, scratch.begin());
      {
        roo_testing::SchedulerSignalGuard callback_guard;
        device->transfer(interface, scratch.data(), bytes * 8);
      }
      if (receive && device == route.receiver) {
        std::copy_n(scratch.begin(), bytes, incoming.begin());
      }
    }
    if (receive) {
      for (size_t i = 0; i < words; ++i) {
        Word value = incoming[sizeof(Word) * i];
        if constexpr (sizeof(Word) == 2) {
          value = static_cast<Word>((value << 8) | incoming[2 * i + 1]);
        }
        dst[offset + i] = value;
      }
    }
    offset += words;
    const uint64_t completed = DurationNs(
        static_cast<uint64_t>(offset) * state.data_bits, state.baudrate);
    roo_testing::pico::TransferDelayNs(completed - elapsed);
    elapsed = completed;
  }
  board.endSpi(index);
  return static_cast<int>(len);
}

}  // namespace

extern "C" {

spi_inst_t* const spi0 = &spi0_instance;
spi_inst_t* const spi1 = &spi1_instance;

uint spi_get_index(const spi_inst_t* spi) { return Index(spi); }

spi_inst_t* spi_get_instance(uint num) {
  CHECK_LT(num, 2u) << "Invalid Pico SPI index";
  return num == 0 ? spi0 : spi1;
}

uint spi_init(spi_inst_t* spi, uint baudrate) {
  roo_testing::pico::ControllerLock controller(Index(spi));
  return FakePico().initSpi(Index(spi), baudrate);
}

void spi_deinit(spi_inst_t* spi) {
  roo_testing::pico::ControllerLock controller(Index(spi));
  FakePico().deinitSpi(Index(spi));
}

uint spi_set_baudrate(spi_inst_t* spi, uint baudrate) {
  roo_testing::pico::ControllerLock controller(Index(spi));
  return FakePico().setSpiBaudrate(Index(spi), baudrate);
}

uint spi_get_baudrate(const spi_inst_t* spi) {
  const FakePicoBoard::SpiState state = FakePico().spiState(Index(spi));
  CHECK(state.enabled) << "Pico SPI is disabled";
  return state.baudrate;
}

void spi_set_format(spi_inst_t* spi, uint bits, spi_cpol_t cpol,
                    spi_cpha_t cpha, spi_order_t order) {
  roo_testing::pico::ControllerLock controller(Index(spi));
  FakePico().setSpiFormat(Index(spi), bits, cpol, cpha, order);
}

void spi_set_slave(spi_inst_t* spi, bool slave) {
  CHECK(FakePico().spiState(Index(spi)).enabled) << "Pico SPI is disabled";
  CHECK(!slave) << "Unimplemented: Pico SPI slave mode";
}

bool spi_is_busy(const spi_inst_t* spi) {
  return FakePico().spiState(Index(spi)).busy;
}

bool spi_is_writable(const spi_inst_t* spi) {
  Index(spi);
  LOG(FATAL) << "Unimplemented: Pico SPI FIFO status";
  return false;
}

bool spi_is_readable(const spi_inst_t* spi) {
  Index(spi);
  LOG(FATAL) << "Unimplemented: Pico SPI FIFO status";
  return false;
}

int spi_write_read_blocking(spi_inst_t* spi, const uint8_t* src, uint8_t* dst,
                            size_t len) {
  return Transfer<uint8_t>(spi, src, dst, len, 0, true, true);
}

int spi_write_blocking(spi_inst_t* spi, const uint8_t* src, size_t len) {
  return Transfer<uint8_t>(spi, src, nullptr, len, 0, true, false);
}

int spi_read_blocking(spi_inst_t* spi, uint8_t repeated, uint8_t* dst,
                      size_t len) {
  return Transfer<uint8_t>(spi, nullptr, dst, len, repeated, false, true);
}

int spi_write16_read16_blocking(spi_inst_t* spi, const uint16_t* src,
                                uint16_t* dst, size_t len) {
  return Transfer<uint16_t>(spi, src, dst, len, 0, true, true);
}

int spi_write16_blocking(spi_inst_t* spi, const uint16_t* src, size_t len) {
  return Transfer<uint16_t>(spi, src, nullptr, len, 0, true, false);
}

int spi_read16_blocking(spi_inst_t* spi, uint16_t repeated, uint16_t* dst,
                        size_t len) {
  return Transfer<uint16_t>(spi, nullptr, dst, len, repeated, false, true);
}

}  // extern "C"
