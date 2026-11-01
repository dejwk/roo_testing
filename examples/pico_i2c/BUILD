load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_binary(
    name = "rtc_example",
    srcs = ["main.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_sdk",
        "//roo_testing/devices/clock/ds3231",
        "//roo_testing/microcontrollers/rp2040:pico",
    ],
)

cc_test(
    name = "rtc_test",
    size = "small",
    srcs = ["main.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        "//:pico_sdk",
        "//roo_testing/devices/clock/ds3231",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
    ],
)
