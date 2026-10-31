load("@roo_testing//roo_testing/emulation:arduino.bzl", "roo_arduino_example")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

roo_arduino_example(
    name = "SntpClock",
    sketch = "SntpClock.ino",
    deps = ["//roo_testing/frameworks/arduino-esp32/cores/esp32"],
)

cc_test(
    name = "sketch_test",
    size = "small",
    srcs = ["test/sketch_test.cpp"],
    target_compatible_with = ["//roo_testing/platforms:arduino"],
    deps = [
        ":SntpClock__ino_context",
        "//:arduino_gtest_main",
    ],
)
