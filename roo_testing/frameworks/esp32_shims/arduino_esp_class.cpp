#include "Esp.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include "esp_arduino_version.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_sleep.h"
#include "esp_system.h"

namespace {
constexpr uint32_t kFlashSize = 16U * 1024U * 1024U;
constexpr uint32_t kSectorSize = 4096;
std::vector<uint8_t> &flashData() {
  static auto *data = new std::vector<uint8_t>(kFlashSize, 0xff);
  return *data;
}
bool flashRangeValid(uint32_t offset, size_t size) {
  return offset <= flashData().size() && size <= flashData().size() - offset;
}
}  // namespace

unsigned long long operator""_kHz(unsigned long long value) { return value * 1000ULL; }
unsigned long long operator""_MHz(unsigned long long value) { return value * 1000000ULL; }
unsigned long long operator""_GHz(unsigned long long value) { return value * 1000000000ULL; }
unsigned long long operator""_kBit(unsigned long long value) { return value * 1024ULL; }
unsigned long long operator""_MBit(unsigned long long value) { return value * 1024ULL * 1024ULL; }
unsigned long long operator""_GBit(unsigned long long value) { return value * 1024ULL * 1024ULL * 1024ULL; }
unsigned long long operator""_kB(unsigned long long value) { return value * 1024ULL; }
unsigned long long operator""_MB(unsigned long long value) { return value * 1024ULL * 1024ULL; }
unsigned long long operator""_GB(unsigned long long value) { return value * 1024ULL * 1024ULL * 1024ULL; }

EspClass ESP;

void EspClass::restart() { esp_restart(); }
uint32_t EspClass::getHeapSize() { return heap_caps_get_total_size(MALLOC_CAP_INTERNAL); }
uint32_t EspClass::getFreeHeap() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }
uint32_t EspClass::getMinFreeHeap() { return heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL); }
uint32_t EspClass::getMaxAllocHeap() { return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL); }
uint32_t EspClass::getPsramSize() { return heap_caps_get_total_size(MALLOC_CAP_SPIRAM); }
uint32_t EspClass::getFreePsram() { return heap_caps_get_free_size(MALLOC_CAP_SPIRAM); }
uint32_t EspClass::getMinFreePsram() { return heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM); }
uint32_t EspClass::getMaxAllocPsram() { return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM); }

uint16_t EspClass::getChipRevision() {
  esp_chip_info_t info{};
  esp_chip_info(&info);
  return info.revision;
}
const char *EspClass::getChipModel() { return "ESP32"; }
uint8_t EspClass::getChipCores() {
  esp_chip_info_t info{};
  esp_chip_info(&info);
  return info.cores;
}
const char *EspClass::getSdkVersion() { return esp_get_idf_version(); }
const char *EspClass::getCoreVersion() { return ESP_ARDUINO_VERSION_STR; }
void EspClass::deepSleep(uint64_t time_us) { esp_deep_sleep(time_us); }

uint32_t EspClass::getFlashChipSize() { return kFlashSize; }
uint32_t EspClass::getFlashChipSpeed() { return 80000000; }
FlashMode_t EspClass::getFlashChipMode() { return FM_QIO; }
uint32_t EspClass::getFlashFrequencyMHz() { return 80; }
uint8_t EspClass::getFlashSourceFrequencyMHz() { return 80; }
uint8_t EspClass::getFlashClockDivider() { return 1; }

uint32_t EspClass::magicFlashChipSize(uint8_t value) {
  return (value & 0x0f) <= 7 ? 1U << (20 + (value & 0x0f)) : 0;
}
uint32_t EspClass::magicFlashChipSpeed(uint8_t value) {
  switch (value & 0x0f) {
    case 0x0: return 40000000;
    case 0x1: return 26000000;
    case 0x2: return 20000000;
    case 0xf: return 80000000;
    default: return 0;
  }
}
FlashMode_t EspClass::magicFlashChipMode(uint8_t value) {
  return value <= FM_SLOW_READ ? static_cast<FlashMode_t>(value) : FM_UNKNOWN;
}

uint32_t EspClass::getSketchSize() { return 0; }
String EspClass::getSketchMD5() { return String("00000000000000000000000000000000"); }
uint32_t EspClass::getFreeSketchSpace() {
  const esp_partition_t *partition = esp_ota_get_next_update_partition(nullptr);
  return partition ? partition->size : 0;
}

bool EspClass::flashEraseSector(uint32_t sector) {
  const uint32_t offset = sector * kSectorSize;
  if (!flashRangeValid(offset, kSectorSize)) return false;
  std::fill_n(flashData().begin() + offset, kSectorSize, 0xff);
  return true;
}
bool EspClass::flashWrite(uint32_t offset, uint32_t *data, size_t size) {
  if (!data || !flashRangeValid(offset, size)) return false;
  std::memcpy(flashData().data() + offset, data, size);
  return true;
}
bool EspClass::flashRead(uint32_t offset, uint32_t *data, size_t size) {
  if (!data || !flashRangeValid(offset, size)) return false;
  std::memcpy(data, flashData().data() + offset, size);
  return true;
}
bool EspClass::partitionEraseRange(const esp_partition_t *partition,
                                   uint32_t offset, size_t size) {
  return esp_partition_erase_range(partition, offset, size) == ESP_OK;
}
bool EspClass::partitionWrite(const esp_partition_t *partition, uint32_t offset,
                              uint32_t *data, size_t size) {
  return esp_partition_write(partition, offset, data, size) == ESP_OK;
}
bool EspClass::partitionRead(const esp_partition_t *partition, uint32_t offset,
                             uint32_t *data, size_t size) {
  return esp_partition_read(partition, offset, data, size) == ESP_OK;
}
uint64_t EspClass::getEfuseMac() {
  uint64_t mac = 0;
  esp_efuse_mac_get_default(reinterpret_cast<uint8_t *>(&mac));
  return mac;
}
