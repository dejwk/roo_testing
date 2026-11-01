load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "pattern",
    srcs = ["pattern.cpp"],
    hdrs = ["pattern.h"],
    copts = ["-fno-exceptions"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_binary(
    name = "display_example",
    srcs = ["main.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        ":pattern",
        "//roo_testing/buses/spi",
        "//roo_testing/devices/display/st77xx:spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/transducers/ui/viewport",
        "@glog",
    ],
)

cc_test(
    name = "display_test",
    size = "small",
    srcs = ["main.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        ":pattern",
        "//roo_testing/buses/spi",
        "//roo_testing/devices/display/st77xx:spi",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
        "//roo_testing/transducers/ui/viewport",
        "@glog",
    ],
)
