#pragma once

// Canonical Arduino additions to the shared classic ESP32/ESP-IDF environment.
// Keep this synchronized with bazelrc/esp32/arduino.bazelrc.

#include "test/profile/esp32_idf_environment_contract.h"

#ifndef ARDUINO
#error "ARDUINO is absent from an unrelated target"
#endif
#if ARDUINO != 10819
#error "ARDUINO has the wrong value"
#endif
#ifndef ARDUINO_ARCH_ESP32
#error "ARDUINO_ARCH_ESP32 is absent from an unrelated target"
#endif
#if ARDUINO_ARCH_ESP32 != 1
#error "ARDUINO_ARCH_ESP32 has the wrong value"
#endif
#ifndef ARDUINO_BOARD
#error "ARDUINO_BOARD is absent from an unrelated target"
#endif
#ifndef ARDUINO_ESP32_DEV
#error "ARDUINO_ESP32_DEV is absent from an unrelated target"
#endif
#if ARDUINO_ESP32_DEV != 1
#error "ARDUINO_ESP32_DEV has the wrong value"
#endif
#ifndef ARDUINO_VARIANT
#error "ARDUINO_VARIANT is absent from an unrelated target"
#endif
#ifndef CORE_DEBUG_LEVEL
#error "CORE_DEBUG_LEVEL is absent from an unrelated target"
#endif
#if CORE_DEBUG_LEVEL != 5
#error "CORE_DEBUG_LEVEL has the wrong value"
#endif

#if !defined(CONFIG_AUTOSTART_ARDUINO) || CONFIG_AUTOSTART_ARDUINO != 1
#error "CONFIG_AUTOSTART_ARDUINO is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_LOOP_STACK_SIZE) || \
    CONFIG_ARDUINO_LOOP_STACK_SIZE != 8192
#error "CONFIG_ARDUINO_LOOP_STACK_SIZE is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_RUNNING_CORE) || CONFIG_ARDUINO_RUNNING_CORE != 0
#error "CONFIG_ARDUINO_RUNNING_CORE is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_EVENT_RUNNING_CORE) || \
    CONFIG_ARDUINO_EVENT_RUNNING_CORE != 0
#error "CONFIG_ARDUINO_EVENT_RUNNING_CORE is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE) || \
    CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE != -1
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE) || \
    CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE != 2048
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE is absent or wrong"
#endif
#if !defined(CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY) || \
    CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY != 24
#error "CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY is absent or wrong"
#endif
#if !defined(CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL) || \
    CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL != 1
#error "CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL is absent or wrong"
#endif
#if !defined(CONFIG_ARDUHAL_ESP_LOG) || CONFIG_ARDUHAL_ESP_LOG != 1
#error "CONFIG_ARDUHAL_ESP_LOG is absent or wrong"
#endif

#ifndef ARDUINO_USB_CDC_ON_BOOT
#error "ARDUINO_USB_CDC_ON_BOOT is absent from an unrelated target"
#endif
#if ARDUINO_USB_CDC_ON_BOOT != 0
#error "ARDUINO_USB_CDC_ON_BOOT has the wrong value"
#endif
#ifndef ARDUINO_USB_DFU_ON_BOOT
#error "ARDUINO_USB_DFU_ON_BOOT is absent from an unrelated target"
#endif
#if ARDUINO_USB_DFU_ON_BOOT != 0
#error "ARDUINO_USB_DFU_ON_BOOT has the wrong value"
#endif
#ifndef ARDUINO_USB_MSC_ON_BOOT
#error "ARDUINO_USB_MSC_ON_BOOT is absent from an unrelated target"
#endif
#if ARDUINO_USB_MSC_ON_BOOT != 0
#error "ARDUINO_USB_MSC_ON_BOOT has the wrong value"
#endif
#ifndef ARDUINO_USB_MODE
#error "ARDUINO_USB_MODE is absent from an unrelated target"
#endif
#if ARDUINO_USB_MODE != 0
#error "ARDUINO_USB_MODE has the wrong value"
#endif
#ifndef ARDUINO_USB_ON_BOOT
#error "ARDUINO_USB_ON_BOOT is absent from an unrelated target"
#endif
#if ARDUINO_USB_ON_BOOT != 0
#error "ARDUINO_USB_ON_BOOT has the wrong value"
#endif
