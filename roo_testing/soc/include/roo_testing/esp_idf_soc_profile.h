#pragma once

// ESP-IDF identity extends the generic SoC contract without importing SDK
// headers.
#ifndef ESP_PLATFORM
#error "ESP-IDF SoC identity requires ESP-IDF capability"
#endif

#if (defined(CONFIG_IDF_TARGET_ESP32) + defined(CONFIG_IDF_TARGET_ESP32S3) + \
     defined(CONFIG_IDF_TARGET_ESP32C3)) != 1
#error "Select exactly one supported ESP-IDF target"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C2) ||  \
    defined(CONFIG_IDF_TARGET_ESP32C5) ||  \
    defined(CONFIG_IDF_TARGET_ESP32C6) ||  \
    defined(CONFIG_IDF_TARGET_ESP32C61) || \
    defined(CONFIG_IDF_TARGET_ESP32H2) ||  \
    defined(CONFIG_IDF_TARGET_ESP32H21) || \
    defined(CONFIG_IDF_TARGET_ESP32H4) ||  \
    defined(CONFIG_IDF_TARGET_ESP32P4) ||  \
    defined(CONFIG_IDF_TARGET_ESP32S2) ||  \
    defined(CONFIG_IDF_TARGET_ESP32S31) || \
    (defined(CONFIG_IDF_TARGET_LINUX) &&   \
     !defined(ROO_TESTING_FREERTOS_POSIX_BACKEND))
#error "Unsupported or contradictory ESP-IDF target"
#endif

#ifdef __cplusplus
namespace roo_testing::soc {

inline constexpr char kIdfTarget[] = CONFIG_IDF_TARGET;
inline constexpr char kIdfArchitecture[] = CONFIG_IDF_TARGET_ARCH;

}  // namespace roo_testing::soc
#endif
