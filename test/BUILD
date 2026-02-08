load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "gpio_test",
    srcs = ["gpio_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/buses/gpio",
        "//roo_testing/transducers/voltage",
        "//:arduino_gtest_main",
    ],
)

cc_test(
    name = "onewire_test",
    srcs = ["onewire_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/buses/onewire",
        "//roo_testing/devices/onewire/thermometer",
        "//roo_testing/transducers/temperature",
        "//:arduino_gtest_main",
    ],
)

cc_test(
    name = "rtc_ds3231_i2c_test",
    srcs = ["rtc_ds3231_i2c_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/devices/clock/ds3231",
        "//:arduino_gtest_main",
    ],
)

cc_test(
    name = "simple_test",
    srcs = ["simple_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/system:timer",
        "//:arduino_gtest_main",
    ],
)

cc_test(
    name = "nvs_test",
    srcs = ["nvs_test.cpp"],
    linkstatic = 1,
    deps = [
        "//roo_testing/microcontrollers/esp32:nvs_storage",
        "//:arduino_gtest_main",
    ],
)
