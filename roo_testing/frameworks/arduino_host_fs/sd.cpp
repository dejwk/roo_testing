// Host adapter for the Arduino-ESP32 SD facade.

#include "SD.h"

#include <utility>

#include "vfs_api.h"

using namespace fs;

namespace {

constexpr uint8_t kHostDrive = 1;
constexpr uint64_t kCardSize = 4ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr uint64_t kTotalBytes = 4ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr uint64_t kUsedBytes = 1ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr size_t kSectorCount = 1024;
constexpr size_t kSectorSize = 1024;

} // namespace

SDFS::SDFS(FSImplPtr impl) : FS(std::move(impl)), _pdrv(0xFF) {}

SDFS::~SDFS() { end(); }

bool SDFS::begin(uint8_t ss_pin, SPIClass &spi, uint32_t frequency,
                 const char *mountpoint, uint8_t max_files,
                 bool format_if_empty) {
  (void)ss_pin;
  (void)frequency;
  (void)max_files;
  (void)format_if_empty;
  if (_pdrv != 0xFF) {
    return true;
  }
  if (!spi.begin()) {
    return false;
  }
  _pdrv = kHostDrive;
  _impl->mountpoint(mountpoint);
  return true;
}

void SDFS::end() {
  if (_pdrv == 0xFF) {
    return;
  }
  _impl->mountpoint(nullptr);
  _pdrv = 0xFF;
}

sdcard_type_t SDFS::cardType() { return _pdrv == 0xFF ? CARD_NONE : CARD_SDHC; }

uint64_t SDFS::cardSize() { return _pdrv == 0xFF ? 0 : kCardSize; }

size_t SDFS::numSectors() { return _pdrv == 0xFF ? 0 : kSectorCount; }

size_t SDFS::sectorSize() { return _pdrv == 0xFF ? 0 : kSectorSize; }

uint64_t SDFS::totalBytes() { return _pdrv == 0xFF ? 0 : kTotalBytes; }

uint64_t SDFS::usedBytes() { return _pdrv == 0xFF ? 0 : kUsedBytes; }

bool SDFS::readRAW(uint8_t *buffer, uint32_t sector) {
  (void)buffer;
  (void)sector;
  return false;
}

bool SDFS::writeRAW(uint8_t *buffer, uint32_t sector) {
  (void)buffer;
  (void)sector;
  return false;
}

SDFS SD = SDFS(FSImplPtr(new VFSImpl()));
