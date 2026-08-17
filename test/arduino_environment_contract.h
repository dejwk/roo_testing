#pragma once

#include "test/idf_environment_contract.h"

#ifndef ARDUINO
#error "Arduino targets must publish their frontend compatibility level"
#endif

#if ARDUINO != 10819
#error "The roo_testing Arduino frontend default must remain documented"
#endif

#ifndef ARDUINO_ARCH_ESP32
#error "Arduino targets must publish their selected architecture"
#endif

#ifndef ARDUINO_ESP32_DEV
#error "Arduino targets must publish their selected board"
#endif

#ifndef ARDUINO_BOARD
#error "Arduino targets must publish ARDUINO_BOARD"
#endif

#ifndef ARDUINO_VARIANT
#error "Arduino targets must publish ARDUINO_VARIANT"
#endif

#ifndef CORE_DEBUG_LEVEL
#error "Arduino targets must publish their logging default"
#endif

static_assert(ARDUINO_ARCH_ESP32 == 1);
static_assert(ARDUINO_ESP32_DEV == 1);
static_assert(CORE_DEBUG_LEVEL == 5);
static_assert(RooTestingEnvironmentStringEquals(ARDUINO_BOARD, "ESP32_DEV"));
static_assert(RooTestingEnvironmentStringEquals(ARDUINO_VARIANT, "esp32"));
