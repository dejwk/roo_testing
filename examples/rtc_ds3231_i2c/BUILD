cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
        "lib/**/*.cpp",  # to compile the 3-rd party DS3231 library.
        "lib/**/*.h",  # likewise.
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
