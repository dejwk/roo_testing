load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

roo_arduino_example(
    name = "peripherals",
    sketch = "Peripherals.ino",
    target_compatible_with = ["//roo_testing/platforms:rp2040"],
    deps = [
        "//:arduino",
        "//roo_testing/buses/uart",
        "//roo_testing/devices/clock/ds3231",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "@glog",
    ],
)

cc_test(
    name = "peripherals_test",
    size = "small",
    srcs = [":peripherals__ino_wrapper"],
    args = ["--iterations=2"],
    linkstatic = True,
    target_compatible_with = [
        "//roo_testing/platforms:rp2040",
        "//roo_testing/platforms:arduino",
    ],
    deps = [
        ":peripherals__ino_context",
        "//:arduino_main",
    ],
)
