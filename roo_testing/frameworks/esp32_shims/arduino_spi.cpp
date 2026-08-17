#include "esp32-hal-spi.h"
#include "esp32-hal-cpu.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-periman.h"

#include <algorithm>
#include <array>
#include <cstring>

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
  }
  return value;
}();

uint16_t swap16(uint16_t value) { return value >> 8 | value << 8; }
uint32_t swap32(uint32_t value) { return __builtin_bswap32(value); }

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
void spiSetClockDiv(spi_t *spi, uint32_t divider) { if (spi) { spi->clock_divider = divider; configureDevice(spi); } }
void spiSetDataMode(spi_t *spi, uint8_t mode) { if (spi) { spi->mode = mode; configureDevice(spi); } }
void spiSetBitOrder(spi_t *spi, uint8_t order) { if (spi) { spi->bit_order = order; configureDevice(spi); } }

void spiWrite(spi_t *spi, const uint32_t *data, uint8_t words) { spiWriteNL(spi, data, std::min<uint8_t>(words, 16) * 4); }
void spiWriteByte(spi_t *spi, uint8_t data) { spiWriteByteNL(spi, data); }
void spiWriteWord(spi_t *spi, uint16_t data) { spiWriteShortNL(spi, data); }
void spiWriteLong(spi_t *spi, uint32_t data) { spiWriteLongNL(spi, data); }
void spiTransfer(spi_t *spi, uint32_t *data, uint8_t words) { spiTransferBytesNL(spi, data, reinterpret_cast<uint8_t *>(data), std::min<uint8_t>(words, 16) * 4); }
uint8_t spiTransferByte(spi_t *spi, uint8_t data) { return spiTransferByteNL(spi, data); }
uint16_t spiTransferWord(spi_t *spi, uint16_t data) { return spiTransferShortNL(spi, data); }
uint32_t spiTransferLong(spi_t *spi, uint32_t data) { return spiTransferLongNL(spi, data); }
void spiTransferBytes(spi_t *spi, const uint8_t *data, uint8_t *out, uint32_t size) { spiTransferBytesNL(spi, data, out, size); }
void spiTransferBits(spi_t *spi, uint32_t data, uint32_t *out, uint8_t bits) { spiTransferBitsNL(spi, data, out, bits); }

void spiTransaction(spi_t *spi, uint32_t divider, uint8_t mode, uint8_t order) { if (spi) { spi->clock_divider = divider; spi->mode = mode; spi->bit_order = order; configureDevice(spi); } }
void spiSimpleTransaction(spi_t *) {}
void spiEndTransaction(spi_t *) {}
void spiWriteNL(spi_t *spi, const void *data, uint32_t length) { transferBytes(spi, static_cast<const uint8_t *>(data), nullptr, length, false); }
void spiWriteByteNL(spi_t *spi, uint8_t data) { spiWriteNL(spi, &data, 1); }
void spiWriteShortNL(spi_t *spi, uint16_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap16(data); spiWriteNL(spi, &data, 2); }
void spiWriteLongNL(spi_t *spi, uint32_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap32(data); spiWriteNL(spi, &data, 4); }
void spiWritePixelsNL(spi_t *spi, const void *data, uint32_t length) { spiWriteNL(spi, data, length); }
uint8_t spiTransferByteNL(spi_t *spi, uint8_t data) { uint8_t out = 0; transferBytes(spi, &data, &out, 1, true); return out; }
uint16_t spiTransferShortNL(spi_t *spi, uint16_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap16(data); uint16_t out = 0; transferBytes(spi, reinterpret_cast<uint8_t *>(&data), reinterpret_cast<uint8_t *>(&out), 2, true); return spi && spi->bit_order == SPI_MSBFIRST ? swap16(out) : out; }
uint32_t spiTransferLongNL(spi_t *spi, uint32_t data) { if (spi && spi->bit_order == SPI_MSBFIRST) data = swap32(data); uint32_t out = 0; transferBytes(spi, reinterpret_cast<uint8_t *>(&data), reinterpret_cast<uint8_t *>(&out), 4, true); return spi && spi->bit_order == SPI_MSBFIRST ? swap32(out) : out; }
void spiTransferBytesNL(spi_t *spi, const void *data, uint8_t *out, uint32_t length) { transferBytes(spi, static_cast<const uint8_t *>(data), out, length, true); }
void spiTransferBitsNL(spi_t *spi, uint32_t data, uint32_t *out, uint8_t bits) {
  if (!spi || !bits || bits > 32) { if (out) *out = 0; return; }
  spi->device->mosi_dlen.usr_mosi_dbitlen = bits - 1;
  spi->device->miso_dlen.usr_miso_dbitlen = bits - 1;
  spi->device->data_buf[0] = data;
  spi->device->cmd.usr = 1;
  if (out) *out = spi->device->data_buf[0];
}

uint32_t spiFrequencyToClockDiv(spi_t *, uint32_t frequency) {
  const uint32_t apb = getApbFrequency();
  if (!frequency || frequency >= apb) return 1u << 31;
  uint32_t best = 0;
  uint64_t best_error = UINT64_MAX;
  for (uint32_t n = 1; n <= 64; ++n) {
    uint32_t pre = std::clamp<uint32_t>(apb / (frequency * n), 1, 8192);
    const uint32_t actual = apb / (pre * n);
    const uint64_t error = actual > frequency ? actual - frequency : frequency - actual;
    if (error < best_error) {
      best_error = error;
      const uint32_t nreg = n - 1;
      best = ((pre - 1) << 18) | (nreg << 12) | ((n / 2 - 1) << 6) | nreg;
    }
  }
  return best;
}
uint32_t spiClockDivToFrequency(spi_t *, uint32_t divider) {
  if (divider & (1u << 31)) return getApbFrequency();
  const uint32_t n = ((divider >> 12) & 0x3f) + 1;
  const uint32_t pre = ((divider >> 18) & 0x1fff) + 1;
  return getApbFrequency() / (n * pre);
}

}  // extern "C"
