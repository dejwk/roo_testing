#pragma once

#ifndef ROO_TESTING_SOC
#error "Select a roo_testing Arduino or ESP-IDF SoC compiler profile"
#endif

#if (defined(ROO_TESTING_SOC_ESP32) + defined(ROO_TESTING_SOC_ESP32S3) + \
     defined(ROO_TESTING_SOC_ESP32C3)) != 1
#error "Select exactly one supported roo_testing SoC"
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

/// Identifies the emulated chip, independently of the native execution host.
enum class Model { kEsp32, kEsp32S3, kEsp32C3 };

#if defined(ROO_TESTING_SOC_ESP32S3)
inline constexpr Model kModel = Model::kEsp32S3;
#elif defined(ROO_TESTING_SOC_ESP32C3)
inline constexpr Model kModel = Model::kEsp32C3;
#else
inline constexpr Model kModel = Model::kEsp32;
#endif
inline constexpr char kName[] = ROO_TESTING_SOC;
inline constexpr char kIdfTarget[] = CONFIG_IDF_TARGET;
inline constexpr char kIdfArchitecture[] = CONFIG_IDF_TARGET_ARCH;

}  // namespace roo_testing::soc
#endif
