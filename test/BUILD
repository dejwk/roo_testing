load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "host_filesystem_test",
    size = "small",
    srcs = ["host_filesystem_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/LittleFS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPIFFS",
        "//roo_testing/microcontrollers/esp32",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "framework_version_test",
    size = "small",
    srcs = ["framework_version_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/frameworks/esp-idf:headers",
    ],
)

cc_test(
    name = "host_event_gateway_test",
    size = "small",
    srcs = ["host_event_gateway_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/host_event",
    ],
)

cc_test(
    name = "gpio_test",
    size = "small",
    srcs = ["gpio_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/buses/gpio",
        "//roo_testing/transducers/voltage",
    ],
)

cc_test(
    name = "onewire_test",
    size = "small",
    srcs = ["onewire_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/buses/onewire",
        "//roo_testing/devices/onewire/thermometer",
        "//roo_testing/transducers/temperature",
    ],
)

cc_test(
    name = "rtc_ds3231_i2c_test",
    size = "small",
    srcs = ["rtc_ds3231_i2c_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/devices/clock/ds3231",
    ],
)

cc_test(
    name = "simple_test",
    size = "small",
    srcs = ["simple_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/system:timer",
    ],
)

cc_test(
    name = "nvs_test",
    size = "small",
    srcs = ["nvs_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/microcontrollers/esp32:nvs_storage",
    ],
)

cc_test(
    name = "freertos_posix_thread_join_regression_test",
    size = "small",
    srcs = ["freertos_posix_thread_join_regression_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino_support:freertos_gtest_main",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
)
