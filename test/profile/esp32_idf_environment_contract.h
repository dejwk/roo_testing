#pragma once

#if !defined(ROO_TESTING) || ROO_TESTING != 1 || !defined(ESP_PLATFORM) || \
    ESP_PLATFORM != 1 || !defined(ESP32) || ESP32 != 1
#error "Missing global emulator/framework identity"
#endif
#if !defined(ROO_TESTING_RUNTIME_FREERTOS) || \
    ROO_TESTING_RUNTIME_FREERTOS != 1 ||      \
    defined(ROO_TESTING_RUNTIME_BARE_METAL)
#error "Missing or contradictory FreeRTOS application runtime identity"
#endif
#ifndef IDF_VER
#error "Missing framework version"
#endif
#if (defined(CONFIG_IDF_TARGET_ESP32) + defined(CONFIG_IDF_TARGET_ESP32S3) + \
     defined(CONFIG_IDF_TARGET_ESP32C3)) != 1
#error "Select exactly one supported chip"
#endif
#if (defined(ROO_TESTING_SOC_ESP32) + defined(ROO_TESTING_SOC_ESP32S3) + \
     defined(ROO_TESTING_SOC_ESP32C3)) != 1
#error "Select exactly one typed chip identity"
#endif
#if defined(CONFIG_IDF_TARGET_LINUX) || \
    defined(ROO_TESTING_FREERTOS_POSIX_BACKEND)
#error "Private host backend identity leaked"
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
#if CONFIG_IDF_TARGET_ESP32 != 1 || !defined(ROO_TESTING_SOC_ESP32) || \
    ROO_TESTING_SOC_ESP32 != 1 || CONFIG_IDF_FIRMWARE_CHIP_ID != 0x0000
#error "Inconsistent chip identity"
#endif
#if !defined(CONFIG_IDF_TARGET_ARCH_XTENSA) || \
    CONFIG_IDF_TARGET_ARCH_XTENSA != 1 ||      \
    defined(CONFIG_IDF_TARGET_ARCH_RISCV)
#error "Inconsistent architecture"
#endif
#define ROO_EXPECTED_SOC "esp32"
#define ROO_EXPECTED_ARCH "xtensa"
#define ROO_EXPECTED_BOARD "ESP32_DEV"
#define ROO_EXPECTED_CORES 2
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#if CONFIG_IDF_TARGET_ESP32S3 != 1 || !defined(ROO_TESTING_SOC_ESP32S3) || \
    ROO_TESTING_SOC_ESP32S3 != 1 || CONFIG_IDF_FIRMWARE_CHIP_ID != 0x0009
#error "Inconsistent chip identity"
#endif
#if !defined(CONFIG_IDF_TARGET_ARCH_XTENSA) || \
    CONFIG_IDF_TARGET_ARCH_XTENSA != 1 ||      \
    defined(CONFIG_IDF_TARGET_ARCH_RISCV)
#error "Inconsistent architecture"
#endif
#define ROO_EXPECTED_SOC "esp32s3"
#define ROO_EXPECTED_ARCH "xtensa"
#define ROO_EXPECTED_BOARD "ESP32S3_DEV"
#define ROO_EXPECTED_CORES 2
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#if CONFIG_IDF_TARGET_ESP32C3 != 1 || !defined(ROO_TESTING_SOC_ESP32C3) || \
    ROO_TESTING_SOC_ESP32C3 != 1 || CONFIG_IDF_FIRMWARE_CHIP_ID != 0x0005
#error "Inconsistent chip identity"
#endif
#if !defined(CONFIG_IDF_TARGET_ARCH_RISCV) || \
    CONFIG_IDF_TARGET_ARCH_RISCV != 1 ||      \
    defined(CONFIG_IDF_TARGET_ARCH_XTENSA)
#error "Inconsistent architecture"
#endif
#define ROO_EXPECTED_SOC "esp32c3"
#define ROO_EXPECTED_ARCH "riscv"
#define ROO_EXPECTED_BOARD "ESP32C3_DEV"
#define ROO_EXPECTED_CORES 1
#endif
