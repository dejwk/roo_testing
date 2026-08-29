load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:arduino",
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["gtest_main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:arduino",
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/system:timer",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "freertos_gtest_main",
    srcs = ["freertos_gtest_main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/system:timer",
        "@googletest//:gtest",
    ],
)

# FreeRTOS-only runner for port and interrupt-controller tests that must start
# before the ESP timer installs its process-global interrupt dispatcher.
cc_library(
    name = "bare_freertos_gtest_main",
    srcs = ["bare_freertos_gtest_main.cpp"],
    alwayslink = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/system:timer",
        "@googletest//:gtest",
    ],
)
