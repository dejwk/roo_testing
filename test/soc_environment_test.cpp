#ifndef ESP32
#error "The SoC profile must publish the canonical ESP32 macro"
#endif

#ifndef CONFIG_IDF_TARGET_ESP32
#error "The SoC profile must publish its ESP-IDF target"
#endif

#ifndef CONFIG_IDF_TARGET_ARCH_XTENSA
#error "The SoC profile must publish its target architecture"
#endif

#ifndef ROO_TESTING_SOC_ESP32
#error "The SoC profile must publish its roo_testing model identity"
#endif

#if defined(ROO_TESTING) || defined(ESP_PLATFORM) || defined(IDF_VER)
#error "The SoC-only profile must not imply a framework or emulator"
#endif

#if defined(ARDUINO) || defined(ARDUINO_ARCH_ESP32) || \
    defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_BOARD) || \
    defined(ARDUINO_VARIANT)
#error "The SoC-only profile must not imply the Arduino frontend"
#endif

int main() { return 0; }
