load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
    ],
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
