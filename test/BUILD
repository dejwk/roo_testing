load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "gpio_test",
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
    srcs = ["rtc_ds3231_i2c_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/devices/clock/ds3231",
    ],
)

cc_test(
    name = "simple_test",
    srcs = ["simple_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/system:timer",
    ],
)

cc_test(
    name = "nvs_test",
    srcs = ["nvs_test.cpp"],
    linkstatic = 1,
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/microcontrollers/esp32:nvs_storage",
    ],
)
