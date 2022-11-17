cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
    ]),
    defines = [
        "ROO_TESTING",
        "ARDUINO=10805",
    ],
    deps = [
        "//lib/roo_display",
        "//roo_testing/devices/display/ili9486:spi",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
        "//roo_testing/transducers/ui/viewport:flex",
        "//roo_testing/transducers/ui/viewport/fltk",
    ],
)
