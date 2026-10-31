#include <string_view>

#if defined(ROO_TESTING_EXPECT_PICO_FRONTEND) && \
    (defined(ARDUINO) || defined(ARDUINO_ARCH_RP2040))
#error "SDK-only Pico frontend must not carry Arduino identity"
#endif

#if defined(ESP_PLATFORM) || defined(ESP32) || defined(IDF_VER) || \
    defined(CONFIG_IDF_TARGET) || defined(ARDUINO_ARCH_ESP32)
#error "Pico SDK profiles must not carry ESP-IDF identity"
#endif
#if defined(ROO_TESTING_SOC_ESP32) || defined(ROO_TESTING_SOC_ESP32S3) || \
    defined(ROO_TESTING_SOC_ESP32C3)
#error "Pico SDK profiles must not carry ESP32 SoC identity"
#endif
#if !defined(PICO_BUILD) || PICO_BUILD != 1 || !defined(PICO_NO_HARDWARE) || \
    PICO_NO_HARDWARE != 1 || !defined(PICO_ON_DEVICE) || PICO_ON_DEVICE != 0
#error "Pico SDK requires native host platform identity"
#endif
#if !defined(PICO_RP2040) || PICO_RP2040 != 1 || !defined(PICO_RP2350) || \
    PICO_RP2350 != 0
#error "Pico SDK compiler identity does not match RP2040"
#endif
static_assert(std::string_view(PICO_PLATFORM_STRING) == "host");
static_assert(std::string_view(PICO_BOARD) == "pico");
