cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
        # "lib/**/*.cpp",
        # "lib/**/*.h",
    ]),
    defines = [
        "ROO_TESTING",
        "ARDUINO=10805",
    ],
    includes = [
        "lib/Arduino-DS3231/src",
    ],
    deps = [
        "@roo_testing//roo_testing/devices/clock/ds3231",
        "@roo_testing//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
    ],
)
