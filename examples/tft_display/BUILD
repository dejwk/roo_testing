load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    deps = [
        "@roo_display",
        "@roo_testing//:arduino_main",
        "@roo_testing//roo_testing/devices/display/ili9486:spi",
        "@roo_testing//roo_testing/transducers/ui/viewport:flex",
        "@roo_testing//roo_testing/transducers/ui/viewport/fltk",
    ],
)
