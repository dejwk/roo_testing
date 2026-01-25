load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    defines = [
        "ROO_TESTING",
        "ARDUINO=10805",
    ],
    deps = [
        "@roo_display",
        "@roo_testing//roo_testing/devices/display/ili9486:spi",
        "@roo_testing//roo_testing/devices/display/ssd1327:spi",
        "@roo_testing//roo_testing/devices/touch/xpt2046:spi",
        "@roo_testing//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
        "@roo_testing//roo_testing/transducers/ui/viewport:flex",
        "@roo_testing//roo_testing/transducers/ui/viewport/fltk",
    ],
)
