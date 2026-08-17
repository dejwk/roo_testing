load("@rules_cc//cc:cc_library.bzl", "cc_library")

alias(
    name = "environment",
    actual = "//roo_testing/frameworks/environment:emulator",
    visibility = ["//visibility:public"],
)

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

# Stable public test entry point for ESP-IDF-only host tests. It starts the
# pthread-backed FreeRTOS scheduler without initializing Arduino.
cc_library(
    name = "esp_idf_gtest_main",
    linkstatic = 1,
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/environment:idf",
    ],
)

# Stable public process entry point for an ESP-IDF application that defines
# extern "C" void app_main().
cc_library(
    name = "esp_idf_main",
    linkstatic = 1,
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/esp_idf_support:main"],
)

test_suite(
    name = "all_tests",
    tests = [
        "//test:arduino_environment_test",
        "//test:arduino_gtest_environment_test",
        "//test:arduino_main_environment_test",
        "//test:arduino_preferences_startup_test",
        "//test:arduino_spi_clock_test",
        "//test:arduino_uart_api_test",
        "//test:emulator_environment_test",
        "//test:framework_version_test",
        "//test:freertos_posix_thread_join_regression_test",
        "//test:gpio_test",
        "//test:host_event_gateway_test",
        "//test:host_filesystem_test",
        "//test:host_network_test",
        "//test:idf_environment_test",
        "//test:nvs_test",
        "//test:onewire_test",
        "//test:rtc_ds3231_i2c_test",
        "//test:simple_test",
        "//test:soc_environment_test",
        "//test:soc_profile_test",
        "//test/legacy_sd_headers:legacy_sd_headers_test",
        "//test/profile:arduino_select_test",
        "//test/profile:global_environment_test",
        "//test/wire_master:wire_master_test",
    ],
)
