load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "blink",
    srcs = ["blink.cpp"],
    hdrs = ["blink.h"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_binary(
    name = "blink_example",
    srcs = ["main.cpp"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [":blink"],
)

cc_test(
    name = "blink_test",
    size = "small",
    srcs = ["blink_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        ":blink",
        "//:pico_gtest_main",
        "//roo_testing/microcontrollers/rp2040:pico",
        "//roo_testing/system:manual_time_mode",
    ],
)
