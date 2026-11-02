load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "task_support",
    hdrs = ["task_support.h"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/system:timer"] + select({
        "//roo_testing/platforms:is_arduino_rp2040_freertos": ["//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "time",
    srcs = ["time.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["gtest_main.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//visibility:public"],
    deps = [
        "//:pico_sdk",
        "//roo_testing/system:timer",
        "@googletest//:gtest",
    ] + select({
        "//roo_testing/platforms:is_arduino_rp2040_freertos": ["//roo_testing/frameworks/freertos_posix:runner"],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "gpio",
    srcs = ["gpio.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "@glog",
    ],
)

cc_library(
    name = "uart",
    srcs = ["uart.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        ":task_support",
        ":time",
        "//roo_testing/buses/uart",
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "@glog",
    ],
)

cc_library(
    name = "spi",
    srcs = ["spi.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        ":task_support",
        "//roo_testing/buses/spi",
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "i2c",
    srcs = ["i2c.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        ":task_support",
        ":time",
        "//roo_testing/buses/i2c",
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "@glog",
    ],
)

cc_library(
    name = "adc",
    srcs = ["adc.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "pwm",
    srcs = ["pwm.cpp"],
    copts = ["-fno-exceptions"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        "//roo_testing/frameworks/pico-sdk:headers",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "sync",
    srcs = ["sync.cpp"],
    visibility = ["//roo_testing/frameworks/pico-sdk:__pkg__"],
    deps = [
        ":task_support",
        "//roo_testing/frameworks/pico-sdk:headers",
        "@glog",
    ],
)
