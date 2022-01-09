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
        "//roo_testing/devices/display/ssd1327:spi",
        "//roo_testing/devices/touch/xpt2046:spi",
        "//roo_testing/frameworks/arduinoespressif32:arduino_main",
        "//roo_testing/transducers/ui/viewport:flex",
        "//roo_testing/transducers/ui/viewport/fltk",
    ],
)
