#pragma once

#include "test/idf_environment_contract.h"

#ifndef ARDUINO
#error "The global build profile must define the Arduino compatibility level"
#endif

#if ARDUINO != 10819
#error "The roo_testing Arduino frontend default must remain documented"
#endif

#ifndef ARDUINO_ARCH_ESP32
#error "The global build profile must define the Arduino architecture"
#endif

#ifndef ARDUINO_ESP32_DEV
#error "The global build profile must define the Arduino board"
#endif

#ifndef ARDUINO_BOARD
#error "The global build profile must define ARDUINO_BOARD"
#endif

#ifndef ARDUINO_VARIANT
#error "The global build profile must define ARDUINO_VARIANT"
#endif

#ifndef CORE_DEBUG_LEVEL
#error "The global build profile must define the logging default"
#endif

static_assert(ARDUINO_ARCH_ESP32 == 1);
static_assert(ARDUINO_ESP32_DEV == 1);
static_assert(CORE_DEBUG_LEVEL == 5);
static_assert(CONFIG_AUTOSTART_ARDUINO == 1);
static_assert(CONFIG_ARDUINO_LOOP_STACK_SIZE == 8192);
static_assert(CONFIG_ARDUINO_RUNNING_CORE == 0);
static_assert(CONFIG_ARDUINO_EVENT_RUNNING_CORE == 0);
static_assert(CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE == -1);
static_assert(CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE == 2048);
static_assert(CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY == 24);
static_assert(CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL == 1);
static_assert(CONFIG_ARDUHAL_ESP_LOG == 1);
static_assert(ARDUINO_USB_CDC_ON_BOOT == 0);
static_assert(ARDUINO_USB_DFU_ON_BOOT == 0);
static_assert(ARDUINO_USB_MSC_ON_BOOT == 0);
static_assert(ARDUINO_USB_MODE == 0);
static_assert(ARDUINO_USB_ON_BOOT == 0);
static_assert(RooTestingEnvironmentStringEquals(ARDUINO_BOARD, "ESP32_DEV"));
static_assert(RooTestingEnvironmentStringEquals(ARDUINO_VARIANT, "esp32"));
