#pragma once

// ESP-IDF's diskio_sdmmc.h uses the FatFs BYTE type, but relies on its own
// callers having already included an internal diskio header.  Host consumers
// include this public header directly, so provide the prerequisite explicitly.
#include "ff.h"

// roo_io 2.2.x used FakeEsp32() after including this header, relying on the
// old monolithic Arduino SD dependency graph for its declaration. The legacy
// SD compatibility target opts into that declaration without broadening the
// modern ESP-IDF header surface.
#if defined(__cplusplus) && defined(ROO_TESTING_LEGACY_SD)
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#endif

#include_next "diskio_sdmmc.h"
