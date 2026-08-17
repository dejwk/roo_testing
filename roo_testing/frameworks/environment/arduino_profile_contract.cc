// Compile guard for the Arduino frontend and its current ESP32 Dev Module
// defaults. Emulator, ESP-IDF, and SoC identity are validated by their layers.

#ifndef ARDUINO
#error "The selected Arduino platform requires the roo_testing compiler profile"
#endif
#if ARDUINO != 10819
#error "ARDUINO has an unsupported compatibility value"
#endif
#ifndef ARDUINO_ARCH_ESP32
#error "The roo_testing Arduino profile must define ARDUINO_ARCH_ESP32"
#endif
#if ARDUINO_ARCH_ESP32 != 1
#error "ARDUINO_ARCH_ESP32 has an unsupported value"
#endif
#ifndef ARDUINO_ESP32_DEV
#error "The roo_testing Arduino profile must define ARDUINO_ESP32_DEV"
#endif
#if ARDUINO_ESP32_DEV != 1
#error "ARDUINO_ESP32_DEV has an unsupported value"
#endif
#ifndef ARDUINO_BOARD
#error "The roo_testing Arduino profile must define ARDUINO_BOARD"
#endif
#ifndef ARDUINO_VARIANT
#error "The roo_testing Arduino profile must define ARDUINO_VARIANT"
#endif
#ifndef CORE_DEBUG_LEVEL
#error "The roo_testing Arduino profile must define CORE_DEBUG_LEVEL"
#endif
#if CORE_DEBUG_LEVEL != 5
#error "CORE_DEBUG_LEVEL has an unsupported value"
#endif

#ifndef CONFIG_AUTOSTART_ARDUINO
#error "The Arduino profile must define CONFIG_AUTOSTART_ARDUINO"
#endif
#if CONFIG_AUTOSTART_ARDUINO != 1
#error "CONFIG_AUTOSTART_ARDUINO has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_LOOP_STACK_SIZE
#error "The Arduino profile must define CONFIG_ARDUINO_LOOP_STACK_SIZE"
#endif
#if CONFIG_ARDUINO_LOOP_STACK_SIZE != 8192
#error "CONFIG_ARDUINO_LOOP_STACK_SIZE has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_RUNNING_CORE
#error "The Arduino profile must define CONFIG_ARDUINO_RUNNING_CORE"
#endif
#if CONFIG_ARDUINO_RUNNING_CORE != 0
#error "CONFIG_ARDUINO_RUNNING_CORE has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_EVENT_RUNNING_CORE
#error "The Arduino profile must define CONFIG_ARDUINO_EVENT_RUNNING_CORE"
#endif
#if CONFIG_ARDUINO_EVENT_RUNNING_CORE != 0
#error "CONFIG_ARDUINO_EVENT_RUNNING_CORE has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE
#error "The Arduino profile must define its serial event task core"
#endif
#if CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE != -1
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE
#error "The Arduino profile must define its serial event task stack size"
#endif
#if CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE != 2048
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE has an unsupported value"
#endif
#ifndef CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY
#error "The Arduino profile must define its serial event task priority"
#endif
#if CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY != 24
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY has an unsupported value"
#endif
#ifndef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
#error "The Arduino profile must define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL"
#endif
#if CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL != 1
#error "CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL has an unsupported value"
#endif
#ifndef CONFIG_ARDUHAL_ESP_LOG
#error "The Arduino profile must define CONFIG_ARDUHAL_ESP_LOG"
#endif
#if CONFIG_ARDUHAL_ESP_LOG != 1
#error "CONFIG_ARDUHAL_ESP_LOG has an unsupported value"
#endif

#ifndef ARDUINO_USB_CDC_ON_BOOT
#error "The roo_testing Arduino profile must define ARDUINO_USB_CDC_ON_BOOT"
#endif
#if ARDUINO_USB_CDC_ON_BOOT != 0
#error "ARDUINO_USB_CDC_ON_BOOT has an unsupported value"
#endif
#ifndef ARDUINO_USB_DFU_ON_BOOT
#error "The roo_testing Arduino profile must define ARDUINO_USB_DFU_ON_BOOT"
#endif
#if ARDUINO_USB_DFU_ON_BOOT != 0
#error "ARDUINO_USB_DFU_ON_BOOT has an unsupported value"
#endif
#ifndef ARDUINO_USB_MSC_ON_BOOT
#error "The roo_testing Arduino profile must define ARDUINO_USB_MSC_ON_BOOT"
#endif
#if ARDUINO_USB_MSC_ON_BOOT != 0
#error "ARDUINO_USB_MSC_ON_BOOT has an unsupported value"
#endif
#ifndef ARDUINO_USB_MODE
#error "The roo_testing Arduino profile must define ARDUINO_USB_MODE"
#endif
#if ARDUINO_USB_MODE != 0
#error "ARDUINO_USB_MODE has an unsupported value"
#endif
#ifndef ARDUINO_USB_ON_BOOT
#error "The roo_testing Arduino profile must define ARDUINO_USB_ON_BOOT"
#endif
#if ARDUINO_USB_ON_BOOT != 0
#error "ARDUINO_USB_ON_BOOT has an unsupported value"
#endif

namespace {

constexpr bool StringEquals(const char* lhs, const char* rhs) {
  while (*lhs != '\0' && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

static_assert(StringEquals(ARDUINO_BOARD, "ESP32_DEV"),
              "ARDUINO_BOARD does not match the emulated board");
static_assert(StringEquals(ARDUINO_VARIANT, "esp32"),
              "ARDUINO_VARIANT does not match the imported variant");

}  // namespace

int roo_testing_arduino_profile_contract() { return 0; }
