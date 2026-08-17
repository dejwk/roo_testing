#pragma once

#include "test/profile/esp32_idf_environment_contract.h"

#if defined(ARDUINO) || defined(ARDUINO_ARCH_ESP32) || \
    defined(ARDUINO_BOARD) || defined(ARDUINO_ESP32_DEV) || \
    defined(ARDUINO_VARIANT) || defined(ARDUINO_USB_CDC_ON_BOOT) || \
    defined(ARDUINO_USB_DFU_ON_BOOT) || defined(ARDUINO_USB_MSC_ON_BOOT) || \
    defined(ARDUINO_USB_MODE) || defined(ARDUINO_USB_ON_BOOT) || \
    defined(CORE_DEBUG_LEVEL) || defined(CONFIG_AUTOSTART_ARDUINO) || \
    defined(CONFIG_ARDUINO_LOOP_STACK_SIZE) || \
    defined(CONFIG_ARDUINO_RUNNING_CORE) || \
    defined(CONFIG_ARDUINO_EVENT_RUNNING_CORE) || \
    defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE) || \
    defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE) || \
    defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY) || \
    defined(CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL) || \
    defined(CONFIG_ARDUHAL_ESP_LOG)
#error "The IDF-only profile must not define Arduino frontend or board macros"
#endif
