#pragma once

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || \
    defined(IDF_VER) || defined(CONFIG_IDF_TARGET) ||              \
    defined(ARDUINO_ARCH_ESP32)
#error "SDK-only Pico profile contains another framework identity"
#endif
#if !defined(ROO_TESTING) || ROO_TESTING != 1 || \
    !defined(ROO_TESTING_RUNTIME_BARE_METAL) ||  \
    ROO_TESTING_RUNTIME_BARE_METAL != 1 ||       \
    defined(ROO_TESTING_RUNTIME_FREERTOS)
#error "SDK-only Pico profile requires bare-metal emulation identity"
#endif
#if !defined(ROO_TESTING_SOC_RP2040) || ROO_TESTING_SOC_RP2040 != 1 || \
    !defined(PICO_NO_HARDWARE) || PICO_NO_HARDWARE != 1 ||             \
    !defined(PICO_ON_DEVICE) || PICO_ON_DEVICE != 0
#error "Pico profile did not reach an unrelated compilation action"
#endif
