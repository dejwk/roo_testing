#pragma once

#ifndef ROO_TESTING
#error "ESP-IDF targets must identify the roo_testing emulator"
#endif

#if ROO_TESTING != 1
#error "ROO_TESTING must have the documented value"
#endif

#ifndef ESP_PLATFORM
#error "ESP-IDF targets must publish ESP_PLATFORM"
#endif

#if ESP_PLATFORM != 1
#error "ESP_PLATFORM must have the documented value"
#endif

#ifndef IDF_VER
#error "ESP-IDF targets must publish IDF_VER"
#endif

#ifndef ESP32
#error "The selected SoC must publish the canonical ESP32 macro"
#endif

#ifndef CONFIG_IDF_TARGET_ESP32
#error "The selected SoC must publish CONFIG_IDF_TARGET_ESP32"
#endif

#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "The selected SoC must publish its architecture"
#endif

#ifndef ROO_TESTING_SOC_ESP32
#error "The selected SoC must publish its roo_testing identity"
#endif

#ifdef CONFIG_IDF_TARGET_LINUX
#error "Linux is the private FreeRTOS backend, not the public ESP-IDF target"
#endif

constexpr bool RooTestingEnvironmentStringEquals(const char* lhs,
                                                  const char* rhs) {
  while (*lhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

static_assert(ESP32 == 1);
static_assert(CONFIG_IDF_TARGET_ESP32 == 1);
static_assert(CONFIG_IDF_TARGET_ARCH_XTENSA == 1);
static_assert(RooTestingEnvironmentStringEquals(IDF_VER, "v6.0.2"));
static_assert(RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET, "esp32"));
static_assert(
    RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET_ARCH, "xtensa"));
