#include "test/idf_environment_contract.h"

#ifdef ARDUINO
#error "The ESP-IDF environment must not imply the Arduino frontend"
#endif

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ESP32_DEV) || \
    defined(ARDUINO_BOARD) || defined(ARDUINO_VARIANT)
#error "The ESP-IDF environment must not publish Arduino board identity"
#endif

int main() { return 0; }
