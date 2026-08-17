#pragma once

#ifndef ROO_TESTING
#error "The global build profile must identify the roo_testing emulator"
#endif

#if ROO_TESTING != 1
#error "ROO_TESTING must have the documented value"
#endif

#ifndef ESP_PLATFORM
#error "The global build profile must define ESP_PLATFORM"
#endif

#if ESP_PLATFORM != 1
#error "ESP_PLATFORM must have the documented value"
#endif

#ifndef IDF_VER
#error "The global build profile must define IDF_VER"
#endif

#ifndef ESP32
#error "The global build profile must define the canonical ESP32 macro"
#endif

#ifndef CONFIG_IDF_TARGET_ESP32
#error "The global build profile must define CONFIG_IDF_TARGET_ESP32"
#endif

#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "The global build profile must define its architecture"
#endif

#ifndef ROO_TESTING_SOC_ESP32
#error "The global build profile must define its roo_testing identity"
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
static_assert(CONFIG_IDF_FIRMWARE_CHIP_ID == 0x0000);
static_assert(ROO_TESTING_SOC_ESP32 == 1);
static_assert(RooTestingEnvironmentStringEquals(IDF_VER, "v6.0.2"));
static_assert(RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET, "esp32"));
static_assert(
    RooTestingEnvironmentStringEquals(CONFIG_IDF_TARGET_ARCH, "xtensa"));
static_assert(RooTestingEnvironmentStringEquals(ROO_TESTING_SOC, "esp32"));
