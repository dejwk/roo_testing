#pragma once

// ESP-IDF's diskio_sdmmc.h uses the FatFs BYTE type, but relies on its own
// callers having already included an internal diskio header.  Host consumers
// include this public header directly, so provide the prerequisite explicitly.
#include "ff.h"
#include_next "diskio_sdmmc.h"
