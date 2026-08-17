#include "Arduino.h"
#include "SD_MMC.h"
#include "diskio_sdmmc.h"
#include "sd_diskio.h"

int main() {
  (void)FakeEsp32().fs_root();
  return 0;
}
