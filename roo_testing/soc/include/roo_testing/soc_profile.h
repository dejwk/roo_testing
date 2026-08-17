#pragma once

// The macros in this header are supplied by //roo_testing/soc:target.  Fail
// loudly when the header is used outside that Bazel target's compile context;
// silently guessing a SoC would make framework feature selection unreliable.
#ifndef ROO_TESTING_SOC
#error "Depend on //roo_testing/soc:target to select the emulated SoC"
#endif

#if !defined(ROO_TESTING_SOC_ESP32) || !defined(CONFIG_IDF_TARGET_ESP32)
#error "The selected roo_testing profile must identify its ESP-IDF SoC"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32C61) || defined(CONFIG_IDF_TARGET_ESP32H2) || \
    defined(CONFIG_IDF_TARGET_ESP32H21) || defined(CONFIG_IDF_TARGET_ESP32H4) || \
    defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S2) || \
    defined(CONFIG_IDF_TARGET_ESP32S3) ||                                  \
    (defined(CONFIG_IDF_TARGET_LINUX) &&                                   \
     !defined(ROO_TESTING_FREERTOS_POSIX_BACKEND))
#error "Multiple ESP-IDF target profiles are active"
#endif

#ifdef __cplusplus
namespace roo_testing::soc {

enum class Model {
  kEsp32,
};

inline constexpr Model kModel = Model::kEsp32;
inline constexpr char kName[] = ROO_TESTING_SOC;
inline constexpr char kIdfTarget[] = CONFIG_IDF_TARGET;
inline constexpr char kIdfArchitecture[] = CONFIG_IDF_TARGET_ARCH;

}  // namespace roo_testing::soc
#endif
