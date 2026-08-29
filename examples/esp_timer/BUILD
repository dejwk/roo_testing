load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "app",
    srcs = ["src/main.cpp"],
    deps = ["//roo_testing/frameworks/esp32_shims:idf"],
)

cc_library(
    name = "arduino_app",
    srcs = ["src/arduino_main.cpp"],
    deps = ["//roo_testing/frameworks/esp32_shims:arduino"],
)

cc_binary(
    name = "main",
    deps = [
        ":arduino_app",
        "//:arduino_main",
    ],
)

cc_binary(
    name = "idf_main",
    deps = [
        ":app",
        "//:esp_idf_main",
    ],
)

cc_test(
    name = "manual_test",
    size = "small",
    srcs = ["esp_timer_example_test.cpp"],
    deps = [
        ":app",
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/system:manual_time_mode",
        "@googletest//:gtest",
    ],
)
