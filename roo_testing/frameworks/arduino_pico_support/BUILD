load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "arduino",
    srcs = [
        "core.cpp",
        "serial.cpp",
        "spi.cpp",
        "wire.cpp",
    ],
    copts = [
        "-fno-exceptions",
        "-fno-rtti",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:pico_sdk",
        "//roo_testing/buses/uart",
        "//roo_testing/frameworks/arduino-pico:headers",
        "//roo_testing/frameworks/arduino-pico:portable",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["gtest_main.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        "//roo_testing/system:timer",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "libc",
    srcs = ["helpers.cpp"],
    visibility = ["//roo_testing/frameworks/arduino-pico:__pkg__"],
    deps = ["//roo_testing/frameworks/arduino-pico:headers"],
)
