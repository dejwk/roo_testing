#if !defined(ARDUINO_ARCH_RP2040) || ARDUINO_ARCH_RP2040 != 1
#error "Arduino-Pico requires ARDUINO_ARCH_RP2040=1"
#endif
#if defined(__FREERTOS) || defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#error \
    "Arduino-Pico native bare-metal profiles cannot carry FreeRTOS/ESP-IDF identity"
#endif
#if !defined(ARDUINO_RASPBERRY_PI_PICO) || F_CPU != 125000000L
#error "Arduino-Pico requires the original Pico variant and nominal core clock"
#endif
