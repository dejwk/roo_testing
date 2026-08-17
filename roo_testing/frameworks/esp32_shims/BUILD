load("@rules_cc//cc:cc_library.bzl", "cc_library")

COMMON_DEFINES = [
    "ARDUINO_ARCH_ESP32",
    "CONFIG_IDF_TARGET_ESP32=1",
    "ESP_PLATFORM",
]

cc_library(
    name = "idf",
    srcs = [
        "esp_now.cpp",
        "event.cpp",
        "idf_core.cpp",
        "idf_drivers.cpp",
        "nvs.cpp",
        "ota_partition.cpp",
        "wifi.cpp",
    ],
    hdrs = ["shim_internal.h"],
    defines = [
    ] + COMMON_DEFINES,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos:freertos",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/sys",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "arduino",
    srcs = [
        "arduino_adc.cpp",
        "arduino_esp_class.cpp",
        "arduino_gpio.cpp",
        "arduino_i2c.cpp",
        "arduino_ledc.cpp",
        "arduino_log.cpp",
        "arduino_misc.cpp",
        "arduino_periman.cpp",
        "arduino_spi.cpp",
        "arduino_time.cpp",
        "arduino_timer.cpp",
        "arduino_uart.cpp",
    ],
    defines = COMMON_DEFINES,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:headers",
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos:freertos",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/system:timer",
    ],
)

cc_library(
    name = "esp32_shims",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        ":idf",
    ],
)
