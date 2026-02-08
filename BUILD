load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "testing",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32",
    ],
)

cc_library(
    name = "arduino_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
    ],
)

cc_library(
    name = "arduino_gtest_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:gtest_main",
    ],
)

test_suite(
    name = "all_tests",
    tests = [
        "//test:gpio_test",
        "//test:nvs_test",
        "//test:onewire_test",
        "//test:rtc_ds3231_i2c_test",
        "//test:simple_test",
    ],
)
