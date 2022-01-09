cc_binary(
    name = "main",
    srcs = glob(["src/**/*.cpp"]) + glob(["src/**/*.h"]),
    defines = [
        "ROO_TESTING",
        "ARDUINO=10805",
    ],
    deps = [
        "//roo_testing/frameworks/arduinoespressif32:arduino_main",
    ],
)
