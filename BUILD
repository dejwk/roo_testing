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
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:arduino",
    ],
)

cc_library(
    name = "arduino_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:main",
    ],
)

cc_library(
    name = "arduino_gtest_main",
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/cores/esp32:gtest_main",
    ],
)

test_suite(
    name = "all_tests",
    tests = [
        "//test:framework_version_test",
        "//test:freertos_posix_thread_join_regression_test",
        "//test:gpio_test",
        "//test:host_event_gateway_test",
        "//test:host_filesystem_test",
        "//test:host_network_test",
        "//test:nvs_test",
        "//test:onewire_test",
        "//test:rtc_ds3231_i2c_test",
        "//test:simple_test",
        "//test:soc_profile_test",
    ],
)
