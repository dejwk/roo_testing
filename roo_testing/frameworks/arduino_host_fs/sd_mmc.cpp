// Host adapter for the Arduino-ESP32 SD_MMC facade.

#include "SD_MMC.h"

#ifdef SOC_SDMMC_HOST_SUPPORTED

#include <utility>

#include "vfs_api.h"

using namespace fs;

namespace {

constexpr uint64_t kCardSize = 4ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr uint64_t kTotalBytes = 4ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr uint64_t kUsedBytes = 1ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr int kSectorCount = 1024;
constexpr int kSectorSize = 1024;

} // namespace

SDMMCFS::SDMMCFS(FSImplPtr impl) : FS(std::move(impl)), _card(nullptr) {}

bool SDMMCFS::sdmmcDetachBus(void *bus_pointer) {
  if (bus_pointer != nullptr) {
    static_cast<SDMMCFS *>(bus_pointer)->end();
  }
  return true;
}

bool SDMMCFS::setPins(int clk, int cmd, int d0) {
  return setPins(clk, cmd, d0, -1, -1, -1);
}

bool SDMMCFS::setPins(int clk, int cmd, int d0, int d1, int d2, int d3) {
  if (_pdrv != 0xFF) {
    log_e("SD_MMC.setPins must be called before SD_MMC.begin");
    return false;
  }
  _pin_clk = static_cast<int8_t>(clk);
  _pin_cmd = static_cast<int8_t>(cmd);
  _pin_d0 = static_cast<int8_t>(d0);
  _pin_d1 = static_cast<int8_t>(d1);
  _pin_d2 = static_cast<int8_t>(d2);
  _pin_d3 = static_cast<int8_t>(d3);
  return true;
}

#ifdef SOC_SDMMC_IO_POWER_EXTERNAL
bool SDMMCFS::setPowerChannel(int power_channel) {
  if (_pdrv != 0xFF) {
    log_e("SD_MMC.setPowerChannel must be called before SD_MMC.begin");
    return false;
  }
  _power_channel = static_cast<int8_t>(power_channel);
  return true;
}
#endif

bool SDMMCFS::begin(const char *mountpoint, bool mode1bit,
                    bool format_if_mount_failed, int sdmmc_frequency,
                    uint8_t max_open_files) {
  (void)format_if_mount_failed;
  (void)sdmmc_frequency;
  (void)max_open_files;
  if (_pdrv != 0xFF) {
    return true;
  }
  _mode1bit = mode1bit;
  _pdrv = 0;
  _impl->mountpoint(mountpoint);
  return true;
}

void SDMMCFS::end() {
  if (_pdrv == 0xFF) {
    return;
  }
  _impl->mountpoint(nullptr);
  _pdrv = 0xFF;
  _card = nullptr;
}

sdcard_type_t SDMMCFS::cardType() {
  return _pdrv == 0xFF ? CARD_NONE : CARD_SDHC;
}

uint64_t SDMMCFS::cardSize() { return _pdrv == 0xFF ? 0 : kCardSize; }

uint64_t SDMMCFS::totalBytes() { return _pdrv == 0xFF ? 0 : kTotalBytes; }

uint64_t SDMMCFS::usedBytes() { return _pdrv == 0xFF ? 0 : kUsedBytes; }

int SDMMCFS::sectorSize() { return _pdrv == 0xFF ? 0 : kSectorSize; }

int SDMMCFS::numSectors() { return _pdrv == 0xFF ? 0 : kSectorCount; }

bool SDMMCFS::readRAW(uint8_t *buffer, uint32_t sector) {
  (void)buffer;
  (void)sector;
  return false;
}

bool SDMMCFS::writeRAW(uint8_t *buffer, uint32_t sector) {
  (void)buffer;
  (void)sector;
  return false;
}

SDMMCFS SD_MMC = SDMMCFS(FSImplPtr(new VFSImpl()));

#endif // SOC_SDMMC_HOST_SUPPORTED
