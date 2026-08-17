#include "esp32-hal-spi.h"
#include "esp32-hal-cpu.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-periman.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

struct spi_struct_t {
  uint8_t number;
  volatile SpiDevType *device;
  uint32_t clock_divider;
  uint8_t mode;
  uint8_t bit_order;
  int8_t sck;
  int8_t miso;
  int8_t mosi;
  std::array<int8_t, 3> ss;
  uint8_t enabled_ss;
  bool ss_inverted;
  bool started;
  std::recursive_mutex *mutex;
};

namespace {
constexpr std::array<uint8_t, 4> kClockSignals = {0, 0, 8, 63};
constexpr std::array<uint8_t, 4> kMisoSignals = {1, 1, 9, 64};
constexpr std::array<uint8_t, 4> kMosiSignals = {2, 2, 10, 65};
std::array<volatile SpiDevType *, 4> devices = {&SPI0, &SPI1, &SPI2, &SPI3};
std::array<spi_t, 4> buses = [] {
  std::array<spi_t, 4> value{};
  for (uint8_t i = 0; i < value.size(); ++i) {
    value[i].number = i;
    value[i].device = devices[i];
    value[i].sck = value[i].miso = value[i].mosi = -1;
    value[i].ss.fill(-1);
    value[i].mutex = new std::recursive_mutex();
  }
  return value;
}();

uint16_t swap16(uint16_t value) { return value >> 8 | value << 8; }
uint32_t swap32(uint32_t value) { return __builtin_bswap32(value); }
uint32_t swap24(uint32_t value) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
  return bytes[2] | static_cast<uint32_t>(bytes[1]) << 8 |
         static_cast<uint32_t>(bytes[0]) << 16;
}
uint32_t swapPixels(uint32_t value) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
  return bytes[1] | static_cast<uint32_t>(bytes[0]) << 8 |
         static_cast<uint32_t>(bytes[3]) << 16 |
         static_cast<uint32_t>(bytes[2]) << 24;
}

void configureDevice(spi_t *spi) {
  spi->device->clock.val = spi->clock_divider;
  spi->device->user.usr_mosi = 1;
  spi->device->user.usr_miso = 1;
  spi->device->user.doutdin = 1;
  spi->device->ctrl.wr_bit_order = spi->bit_order == SPI_LSBFIRST;
  spi->device->ctrl.rd_bit_order = spi->bit_order == SPI_LSBFIRST;
  switch (spi->mode) {
    case SPI_MODE1: spi->device->pin.ck_idle_edge = 0; spi->device->user.ck_out_edge = 1; break;
    case SPI_MODE2: spi->device->pin.ck_idle_edge = 1; spi->device->user.ck_out_edge = 1; break;
    case SPI_MODE3: spi->device->pin.ck_idle_edge = 1; spi->device->user.ck_out_edge = 0; break;
    default: spi->device->pin.ck_idle_edge = 0; spi->device->user.ck_out_edge = 0; break;
  }
}

void transferChunk(spi_t *spi, const uint8_t *input, uint8_t *output,
                   uint32_t length, bool read) {
  std::array<uint32_t, 16> words{};
  if (input) std::memcpy(words.data(), input, length);
  else std::memset(words.data(), 0xff, length);
  const size_t word_count = (length + 3) / 4;
  for (size_t i = 0; i < word_count; ++i) spi->device->data_buf[i] = words[i];
  spi->device->mosi_dlen.usr_mosi_dbitlen = length * 8 - 1;
  spi->device->miso_dlen.usr_miso_dbitlen = read ? length * 8 - 1 : 0;
  spi->device->cmd.usr = 1;
  if (output) {
    for (size_t i = 0; i < word_count; ++i) words[i] = spi->device->data_buf[i];
    std::memcpy(output, words.data(), length);
  }
}

void transferBytes(spi_t *spi, const uint8_t *input, uint8_t *output,
                   uint32_t length, bool read) {
  if (!spi || !spi->started) return;
  while (length) {
    const uint32_t chunk = std::min<uint32_t>(64, length);
    transferChunk(spi, input, output, chunk, read);
    if (input) input += chunk;
    if (output) output += chunk;
    length -= chunk;
  }
}
}  // namespace

extern "C" {

spi_t *spiStartBus(uint8_t number, uint32_t divider, uint8_t mode, uint8_t order) {
  if (number >= buses.size()) return nullptr;
  spi_t *spi = &buses[number];
  spi->clock_divider = divider;
  spi->mode = mode;
  spi->bit_order = order;
  spi->started = true;
  configureDevice(spi);
  return spi;
}
void spiStopBus(spi_t *spi) { if (spi) spi->started = false; }

bool spiAttachSCK(spi_t *spi, int8_t pin) {
  if (!spi || pin < 0) return false;
  if (spi->sck >= 0) perimanClearPinBus(spi->sck);
  spi->sck = pin;
  FakeEsp32().out_matrix.assign(pin, kClockSignals[spi->number], false, false);
  return perimanSetPinBus(pin, ESP32_BUS_TYPE_SPI_MASTER_SCK, spi, spi->number, -1);
}
bool spiAttachMISO(spi_t *spi, int8_t pin) {
  if (!spi || pin < 0) return false;
  if (spi->miso >= 0) perimanClearPinBus(spi->miso);
  spi->miso = pin;
  FakeEsp32().in_matrix.assign(pin, kMisoSignals[spi->number], false);
  return perimanSetPinBus(pin, ESP32_BUS_TYPE_SPI_MASTER_MISO, spi, spi->number, -1);
}
bool spiAttachMOSI(spi_t *spi, int8_t pin) {
  if (!spi || pin < 0) return false;
  if (spi->mosi >= 0) perimanClearPinBus(spi->mosi);
  spi->mosi = pin;
  FakeEsp32().out_matrix.assign(pin, kMosiSignals[spi->number], false, false);
  return perimanSetPinBus(pin, ESP32_BUS_TYPE_SPI_MASTER_MOSI, spi, spi->number, -1);
}
bool spiDetachSCK(spi_t *spi) { if (!spi || spi->sck < 0) return false; perimanClearPinBus(spi->sck); spi->sck = -1; return true; }
bool spiDetachMISO(spi_t *spi) { if (!spi || spi->miso < 0) return false; perimanClearPinBus(spi->miso); spi->miso = -1; return true; }
bool spiDetachMOSI(spi_t *spi) { if (!spi || spi->mosi < 0) return false; perimanClearPinBus(spi->mosi); spi->mosi = -1; return true; }

bool spiAttachSS(spi_t *spi, uint8_t number, int8_t pin) {
  if (!spi || number >= spi->ss.size() || pin < 0) return false;
  spi->ss[number] = pin;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, spi->ss_inverted ? LOW : HIGH);
  return perimanSetPinBus(pin, ESP32_BUS_TYPE_SPI_MASTER_SS, spi, spi->number, number);
}
bool spiDetachSS(spi_t *spi) {
  if (!spi) return false;
  bool detached = false;
  for (int8_t &pin : spi->ss) if (pin >= 0) { perimanClearPinBus(pin); pin = -1; detached = true; }
  return detached;
}
void spiEnableSSPins(spi_t *spi, uint8_t mask) { if (spi) spi->enabled_ss |= mask; }
void spiDisableSSPins(spi_t *spi, uint8_t mask) { if (spi) spi->enabled_ss &= ~mask; }
void spiSSEnable(spi_t *spi) { if (spi) spi->enabled_ss = SPI_SS_MASK_ALL; }
void spiSSDisable(spi_t *spi) { if (spi) spi->enabled_ss = 0; }
void spiSSSet(spi_t *spi) { if (spi) for (uint8_t i = 0; i < spi->ss.size(); ++i) if ((spi->enabled_ss & (1 << i)) && spi->ss[i] >= 0) digitalWrite(spi->ss[i], spi->ss_inverted ? HIGH : LOW); }
void spiSSClear(spi_t *spi) { if (spi) for (uint8_t i = 0; i < spi->ss.size(); ++i) if ((spi->enabled_ss & (1 << i)) && spi->ss[i] >= 0) digitalWrite(spi->ss[i], spi->ss_inverted ? LOW : HIGH); }
void spiWaitReady(spi_t *) {}
void spiSSInvert(spi_t *spi, bool inverted) { if (spi) spi->ss_inverted = inverted; }

uint32_t spiGetClockDiv(spi_t *spi) { return spi ? spi->clock_divider : 0; }
uint8_t spiGetDataMode(spi_t *spi) { return spi ? spi->mode : SPI_MODE0; }
uint8_t spiGetBitOrder(spi_t *spi) { return spi ? spi->bit_order : SPI_MSBFIRST; }
void spiSetClockDiv(spi_t *spi, uint32_t divider) { if (spi) { std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spi->clock_divider = divider; configureDevice(spi); } }
void spiSetDataMode(spi_t *spi, uint8_t mode) { if (spi) { std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spi->mode = mode; configureDevice(spi); } }
void spiSetBitOrder(spi_t *spi, uint8_t order) { if (spi) { std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spi->bit_order = order; configureDevice(spi); } }

void spiWrite(spi_t *spi, const uint32_t *data, uint8_t words) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiWriteNL(spi, data, std::min<uint8_t>(words, 16) * 4); }
void spiWriteByte(spi_t *spi, uint8_t data) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiWriteByteNL(spi, data); }
void spiWriteWord(spi_t *spi, uint16_t data) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiWriteShortNL(spi, data); }
void spiWriteLong(spi_t *spi, uint32_t data) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiWriteLongNL(spi, data); }
void spiTransfer(spi_t *spi, uint32_t *data, uint8_t words) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiTransferBytesNL(spi, data, reinterpret_cast<uint8_t *>(data), std::min<uint8_t>(words, 16) * 4); }
uint8_t spiTransferByte(spi_t *spi, uint8_t data) { if (!spi) return 0; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); return spiTransferByteNL(spi, data); }
uint16_t spiTransferWord(spi_t *spi, uint16_t data) { if (!spi) return 0; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); return spiTransferShortNL(spi, data); }
uint32_t spiTransferLong(spi_t *spi, uint32_t data) { if (!spi) return 0; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); return spiTransferLongNL(spi, data); }
void spiTransferBytes(spi_t *spi, const uint8_t *data, uint8_t *out, uint32_t size) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiTransferBytesNL(spi, data, out, size); }
void spiTransferBits(spi_t *spi, uint32_t data, uint32_t *out, uint8_t bits) { if (!spi) return; std::lock_guard<std::recursive_mutex> lock(*spi->mutex); spiTransferBitsNL(spi, data, out, bits); }

void spiTransaction(spi_t *spi, uint32_t divider, uint8_t mode, uint8_t order) { if (spi) { spi->mutex->lock(); spi->clock_divider = divider; spi->mode = mode; spi->bit_order = order; configureDevice(spi); } }
void spiSimpleTransaction(spi_t *spi) { if (spi) spi->mutex->lock(); }
void spiEndTransaction(spi_t *spi) { if (spi) spi->mutex->unlock(); }
void spiWriteNL(spi_t *spi, const void *data, uint32_t length) { transferBytes(spi, static_cast<const uint8_t *>(data), nullptr, length, false); }
void spiWriteByteNL(spi_t *spi, uint8_t data) { spiWriteNL(spi, &data, 1); }
void spiWriteShortNL(spi_t *spi, uint16_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap16(data); spiWriteNL(spi, &data, 2); }
void spiWriteLongNL(spi_t *spi, uint32_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap32(data); spiWriteNL(spi, &data, 4); }
void spiWritePixelsNL(spi_t *spi, const void *input, uint32_t length) {
  if (!spi || !input) return;
  const auto *data = static_cast<const uint32_t *>(input);
  size_t words_left = (length + 3) / 4;
  while (length) {
    const uint32_t chunk = std::min<uint32_t>(64, length);
    const size_t words = std::min<size_t>(16, words_left);
    const size_t tail = chunk & 3;
    spi->device->mosi_dlen.usr_mosi_dbitlen = chunk * 8 - 1;
    spi->device->miso_dlen.usr_miso_dbitlen = 0;
    for (size_t i = 0; i < words; ++i) {
      uint32_t value = data[i];
      if (spi->bit_order == SPI_MSBFIRST) {
        if (tail && i + 1 == words) {
          value = tail == 2 ? swap16(static_cast<uint16_t>(value)) : value & 0xff;
        } else {
          value = swapPixels(value);
        }
      }
      spi->device->data_buf[i] = value;
    }
    spi->device->cmd.usr = 1;
    data += words;
    words_left -= words;
    length -= chunk;
  }
}
uint8_t spiTransferByteNL(spi_t *spi, uint8_t data) { uint8_t out = 0; transferBytes(spi, &data, &out, 1, true); return out; }
uint16_t spiTransferShortNL(spi_t *spi, uint16_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap16(data); uint16_t out = 0; transferBytes(spi, reinterpret_cast<uint8_t *>(&data), reinterpret_cast<uint8_t *>(&out), 2, true); return spi && spi->bit_order == SPI_MSBFIRST ? swap16(out) : out; }
uint32_t spiTransferLongNL(spi_t *spi, uint32_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap32(data); uint32_t out = 0; transferBytes(spi, reinterpret_cast<uint8_t *>(&data), reinterpret_cast<uint8_t *>(&out), 4, true); return spi && spi->bit_order == SPI_MSBFIRST ? swap32(out) : out; }
void spiTransferBytesNL(spi_t *spi, const void *data, uint8_t *out, uint32_t length) { transferBytes(spi, static_cast<const uint8_t *>(data), out, length, true); }
void spiTransferBitsNL(spi_t *spi, uint32_t data, uint32_t *out, uint8_t bits) {
  if (!spi || !bits) { if (out) *out = 0; return; }
  bits = std::min<uint8_t>(bits, 32);
  const uint32_t bytes = (bits + 7) / 8;
  const uint32_t mask = bits == 32 ? UINT32_MAX : (uint32_t{1} << bits) - 1;
  data &= mask;
  if (spi->bit_order == SPI_MSBFIRST) {
    data = bytes == 2 ? swap16(static_cast<uint16_t>(data))
         : bytes == 3 ? swap24(data) : swap32(data);
  }
  spi->device->mosi_dlen.usr_mosi_dbitlen = bits - 1;
  spi->device->miso_dlen.usr_miso_dbitlen = bits - 1;
  spi->device->data_buf[0] = data;
  spi->device->cmd.usr = 1;
  if (out) {
    data = spi->device->data_buf[0];
    *out = spi->bit_order == SPI_MSBFIRST
               ? (bytes == 2 ? swap16(static_cast<uint16_t>(data))
                  : bytes == 3 ? swap24(data) : swap32(data))
               : data;
    *out &= mask;
  }
}

uint32_t spiFrequencyToClockDiv(spi_t *, uint32_t frequency) {
  union ClockDivider {
    uint32_t value;
    struct {
      uint32_t clkcnt_l : 6;
      uint32_t clkcnt_h : 6;
      uint32_t clkcnt_n : 6;
      uint32_t clkdiv_pre : 13;
      uint32_t clk_equ_sysclk : 1;
    };
  };

  const uint32_t source_frequency = getApbFrequency();
  if (frequency >= source_frequency) return 1U << 31;

  ClockDivider minimum{};
  minimum.value = 0x7ffff000;
  const uint32_t minimum_frequency =
      source_frequency /
      ((minimum.clkdiv_pre + 1) * (minimum.clkcnt_n + 1));
  if (frequency < minimum_frequency) return minimum.value;

  uint8_t clock_count = 1;
  ClockDivider best{};
  uint32_t best_frequency = 0;
  while (clock_count <= 0x3f) {
    ClockDivider candidate{};
    candidate.clkcnt_n = clock_count;
    uint32_t candidate_frequency = 0;
    int8_t pre_variation = -2;
    while (pre_variation++ <= 1) {
      int32_t pre =
          static_cast<int32_t>((source_frequency /
                                (candidate.clkcnt_n + 1)) /
                               frequency) -
          1 + pre_variation;
      if (pre > 0x1fff) {
        candidate.clkdiv_pre = 0x1fff;
      } else if (pre <= 0) {
        candidate.clkdiv_pre = 0;
      } else {
        candidate.clkdiv_pre = pre;
      }
      candidate.clkcnt_l = (candidate.clkcnt_n + 1) / 2;
      candidate_frequency =
          source_frequency /
          ((candidate.clkdiv_pre + 1) * (candidate.clkcnt_n + 1));
      if (candidate_frequency == frequency) {
        best = candidate;
        break;
      }
      if (candidate_frequency < frequency &&
          (best_frequency == 0 ||
           frequency - candidate_frequency < frequency - best_frequency)) {
        best_frequency = candidate_frequency;
        best = candidate;
      }
    }
    if (candidate_frequency == frequency) break;
    ++clock_count;
  }
  return best.value;
}
uint32_t spiClockDivToFrequency(spi_t *, uint32_t divider) {
  if (divider & (1u << 31)) return getApbFrequency();
  const uint32_t n = ((divider >> 12) & 0x3f) + 1;
  const uint32_t pre = ((divider >> 18) & 0x1fff) + 1;
  return getApbFrequency() / (n * pre);
}

}  // extern "C"
