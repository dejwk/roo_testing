#include <string_view>

#include "roo_testing/soc_profile.h"

#ifndef CONFIG_IDF_TARGET_ESP32C3
#error "Compiler identity does not match selected SoC target"
#endif
#ifndef ROO_TESTING_SOC_ESP32C3
#error "Typed identity does not match selected SoC target"
#endif
#if !defined(CONFIG_IDF_TARGET_ARCH_RISCV) || CONFIG_IDF_TARGET_ARCH_RISCV != 1
#error "Incorrect emulated architecture"
#endif
#ifdef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "Contradictory architecture"
#endif
static_assert(CONFIG_IDF_TARGET_ESP32C3 == 1);
static_assert(ROO_TESTING_SOC_ESP32C3 == 1);
static_assert(ESP32 == 1);
static_assert(CONFIG_IDF_FIRMWARE_CHIP_ID == 0x0005);
static_assert(std::string_view(CONFIG_IDF_TARGET) == "esp32c3");
static_assert(std::string_view(CONFIG_IDF_TARGET_ARCH) == "riscv");
static_assert(std::string_view(ROO_TESTING_SOC) == "esp32c3");
