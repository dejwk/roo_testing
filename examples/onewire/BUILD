cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
        "lib/**/*.cpp",
        "lib/**/*.h",
    ]),
    defines = [
        "ROO_TESTING",
        "ARDUINO=10805",
    ],
    includes = [
        "lib/Arduino-Temperature-Control-Library-3.9.1",
        "roo_testing/buses/onewire",
    ],
    deps = [
        "//roo_testing/buses/onewire",
        "//roo_testing/devices/onewire/thermometer",
        "//roo_testing/frameworks/arduinoespressif32:arduino",
    ],
)
