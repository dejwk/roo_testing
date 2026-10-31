#include <string_view>

#include "roo_testing/soc_profile.h"

#ifndef CONFIG_IDF_TARGET_ESP32
#error "Compiler identity does not match selected SoC target"
#endif
#ifndef ROO_TESTING_SOC_ESP32
#error "Typed identity does not match selected SoC target"
#endif
#if !defined(CONFIG_IDF_TARGET_ARCH_XTENSA) || \
    CONFIG_IDF_TARGET_ARCH_XTENSA != 1
#error "Incorrect emulated architecture"
#endif
#ifdef CONFIG_IDF_TARGET_ARCH_RISCV
#error "Contradictory architecture"
#endif
static_assert(CONFIG_IDF_TARGET_ESP32 == 1);
static_assert(ROO_TESTING_SOC_ESP32 == 1);
static_assert(ESP32 == 1);
static_assert(CONFIG_IDF_FIRMWARE_CHIP_ID == 0x0000);
static_assert(std::string_view(CONFIG_IDF_TARGET) == "esp32");
static_assert(std::string_view(CONFIG_IDF_TARGET_ARCH) == "xtensa");
static_assert(std::string_view(ROO_TESTING_SOC) == "esp32");
