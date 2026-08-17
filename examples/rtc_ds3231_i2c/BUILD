load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = glob(
        [
            "src/**/*.cpp",
            "lib/**/src/*.h",
            "lib/**/src/*.cpp",
        ],
        # lib/init.sh populates Arduino-DS3231 for this standalone example.
        allow_empty = True,
    ),
    includes = [
        "lib/Arduino-DS3231/src",
    ],
    deps = [
        "@roo_testing//roo_testing/devices/clock/ds3231",
        "@roo_testing//roo_testing/frameworks/arduino-esp32/cores/esp32:main",
    ],
)
