load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "arduino_test",
    size = "small",
    srcs = ["arduino_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
        "-fno-rtti",
    ],
    linkstatic = True,
    target_compatible_with = [
        "//roo_testing/platforms:bare_metal",
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:arduino",
    ],
    deps = [
        "//:arduino_gtest_main",
        "//roo_testing/buses/uart",
        "//roo_testing/devices/clock/ds3231",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/system:timer",
    ],
)

cc_binary(
    name = "unsupported_core1_probe",
    srcs = ["unsupported_core1_probe.cpp"],
    linkstatic = True,
    tags = ["manual"],
    deps = ["//:arduino"],
)

cc_binary(
    name = "unsupported_freertos_probe",
    srcs = ["unsupported_freertos_probe.cpp"],
    linkstatic = True,
    tags = ["manual"],
    deps = ["//:arduino"],
)

cc_binary(
    name = "console_probe",
    srcs = ["console_probe.cpp"],
    linkstatic = True,
    tags = ["manual"],
    deps = ["//:arduino"],
)
