#pragma once

// Canonical macros shared by ESP-IDF-only and Arduino builds on the classic
// ESP32 emulator profile. Keep this synchronized with
// .roo_testing/bazelrc/esp32/base.bazelrc.

#ifndef ROO_TESTING
#error "ROO_TESTING is absent from an unrelated target"
#endif
#if ROO_TESTING != 1
#error "ROO_TESTING has the wrong value"
#endif

#ifndef ESP_PLATFORM
#error "ESP_PLATFORM is absent from an unrelated target"
#endif
#if ESP_PLATFORM != 1
#error "ESP_PLATFORM has the wrong value"
#endif
#ifndef IDF_VER
#error "IDF_VER is absent from an unrelated target"
#endif

#ifndef CONFIG_IDF_FIRMWARE_CHIP_ID
#error "CONFIG_IDF_FIRMWARE_CHIP_ID is absent from an unrelated target"
#endif
#if CONFIG_IDF_FIRMWARE_CHIP_ID != 0x0000
#error "CONFIG_IDF_FIRMWARE_CHIP_ID has the wrong value"
#endif
#ifndef CONFIG_IDF_TARGET
#error "CONFIG_IDF_TARGET is absent from an unrelated target"
#endif
#ifndef CONFIG_IDF_TARGET_ARCH
#error "CONFIG_IDF_TARGET_ARCH is absent from an unrelated target"
#endif
#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "CONFIG_IDF_TARGET_ARCH_XTENSA is absent from an unrelated target"
#endif
#if CONFIG_IDF_TARGET_ARCH_XTENSA != 1
#error "CONFIG_IDF_TARGET_ARCH_XTENSA has the wrong value"
#endif
#ifndef CONFIG_IDF_TARGET_ESP32
#error "CONFIG_IDF_TARGET_ESP32 is absent from an unrelated target"
#endif
#if CONFIG_IDF_TARGET_ESP32 != 1
#error "CONFIG_IDF_TARGET_ESP32 has the wrong value"
#endif
#ifndef ESP32
#error "ESP32 is absent from an unrelated target"
#endif
#if ESP32 != 1
#error "ESP32 has the wrong value"
#endif
#ifndef ROO_TESTING_SOC
#error "ROO_TESTING_SOC is absent from an unrelated target"
#endif
#ifndef ROO_TESTING_SOC_ESP32
#error "ROO_TESTING_SOC_ESP32 is absent from an unrelated target"
#endif
#if ROO_TESTING_SOC_ESP32 != 1
#error "ROO_TESTING_SOC_ESP32 has the wrong value"
#endif

#if defined(CONFIG_IDF_TARGET_LINUX) || defined(CONFIG_IDF_TARGET_ARCH_RISCV) || \
    defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32C61) || defined(CONFIG_IDF_TARGET_ESP32H2) || \
    defined(CONFIG_IDF_TARGET_ESP32H21) || defined(CONFIG_IDF_TARGET_ESP32H4) || \
    defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S2) || \
    defined(CONFIG_IDF_TARGET_ESP32S3)
#error "The global compiler profile contains contradictory target macros"
#endif
