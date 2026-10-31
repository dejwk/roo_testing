#include <string_view>

#include "roo_testing/soc_profile.h"

#if defined(ROO_TESTING_EXPECT_SOC_ESP32)
#ifndef ROO_TESTING_SOC_ESP32
#error "Typed identity does not match selected SoC platform"
#endif
static_assert(ROO_TESTING_SOC_ESP32 == 1);
static_assert(std::string_view(ROO_TESTING_SOC) == "esp32");
#elif defined(ROO_TESTING_EXPECT_SOC_ESP32S3)
#ifndef ROO_TESTING_SOC_ESP32S3
#error "Typed identity does not match selected SoC platform"
#endif
static_assert(ROO_TESTING_SOC_ESP32S3 == 1);
static_assert(std::string_view(ROO_TESTING_SOC) == "esp32s3");
#elif defined(ROO_TESTING_EXPECT_SOC_ESP32C3)
#ifndef ROO_TESTING_SOC_ESP32C3
#error "Typed identity does not match selected SoC platform"
#endif
static_assert(ROO_TESTING_SOC_ESP32C3 == 1);
static_assert(std::string_view(ROO_TESTING_SOC) == "esp32c3");
#endif
