#if !ROO_TESTING || !ROO_TESTING_SOC_RP2040 ||                      \
    !ROO_TESTING_RUNTIME_FREERTOS || !__FREERTOS || !PICO_RP2040 || \
    PICO_RP2350 || ARDUINO != 10819
#error Incorrect global Pico FreeRTOS C identity
#endif
#if defined(ESP32) || defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32) || \
    defined(ROO_TESTING_RUNTIME_BARE_METAL) || defined(CONFIG_IDF_TARGET)
#error Foreign C identity
#endif
int PicoFreeRtosCIdentity(void) { return 1; }
