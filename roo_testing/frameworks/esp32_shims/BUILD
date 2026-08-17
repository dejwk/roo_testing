load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

COMMON_DEFINES = [
    "ESP_PLATFORM",
]

# Architecture-independent sources kept current by the ESP-IDF import.  They
# live in the existing destination-owned shim package so adding host build
# metadata does not turn imported component directories into Bazel packages.
cc_library(
    name = "esp_err_to_name",
    srcs = [
        "//roo_testing/frameworks/esp-idf:components/esp_common/src/esp_err_to_name.c",
    ],
    local_defines = ["CONFIG_ESP_ERR_TO_NAME_LOOKUP=1"],
    deps = ["//roo_testing/frameworks/esp-idf:headers"],
)

cc_library(
    name = "esp_ringbuf",
    srcs = [
        "//roo_testing/frameworks/esp-idf:components/esp_ringbuf/ringbuf.c",
    ],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos:freertos",
    ],
)

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
        ":esp_err_to_name",
        ":esp_ringbuf",
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

cc_test(
    name = "runtime_compat_test",
    size = "small",
    srcs = ["runtime_compat_test.cpp"],
    linkstatic = 1,
    deps = [
        ":idf",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "@googletest//:gtest",
    ],
)
