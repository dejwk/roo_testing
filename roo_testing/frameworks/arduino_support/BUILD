load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["gtest_main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "@googletest//:gtest",
    ],
)
