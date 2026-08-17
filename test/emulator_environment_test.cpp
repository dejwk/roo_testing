#ifndef ROO_TESTING
#error "The emulator environment must publish ROO_TESTING"
#endif

#if ROO_TESTING != 1
#error "ROO_TESTING must have the documented value"
#endif

#ifdef ESP_PLATFORM
#error "The emulator-only environment must not imply ESP-IDF"
#endif

#ifdef ARDUINO
#error "The emulator-only environment must not imply Arduino"
#endif

#ifdef ESP32
#error "The emulator-only environment must not imply a selected SoC"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32
#error "The emulator-only environment must not imply an ESP-IDF target"
#endif

int main() { return 0; }
