// Host compatibility for Arduino-ESP32's low-level SD teardown API.

#include "sd_diskio.h"

// The host SD facade mounts a POSIX directory directly, so it never registers
// an Arduino diskio drive. Match upstream's response for an unknown drive.
uint8_t sdcard_unmount(uint8_t pdrv) {
  (void)pdrv;
  return 1;
}

uint8_t sdcard_uninit(uint8_t pdrv) {
  (void)pdrv;
  return 1;
}
