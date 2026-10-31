#pragma once

#ifndef ROO_TESTING_SOC
#error "Select a roo_testing SoC compiler profile"
#endif

#if (defined(ROO_TESTING_SOC_ESP32) + defined(ROO_TESTING_SOC_ESP32S3) + \
     defined(ROO_TESTING_SOC_ESP32C3)) != 1
#error "Select exactly one supported roo_testing SoC"
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

}  // namespace roo_testing::soc
#endif

// Preserve existing IDF constants for ESP-IDF-capable consumers without making
// generic identity require an IDF compiler environment.
#if defined(ESP_PLATFORM)
#include "roo_testing/esp_idf_soc_profile.h"
#endif
