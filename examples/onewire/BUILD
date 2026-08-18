load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = glob([
        "src/**/*.cpp",
        # "src/**/*.h",
        # "lib/**/*.cpp",
        # "lib/**/*.h",
    ]),
    includes = [
        "lib/Arduino-Temperature-Control-Library-3.9.1",
    ],
    deps = [
        "@roo_testing//:arduino_main",
        "@roo_testing//roo_testing/buses/onewire",
        "@roo_testing//roo_testing/devices/onewire/thermometer",
    ],
)
