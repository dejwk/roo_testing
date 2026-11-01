load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "uart",
    srcs = ["fake_uart.cpp"],
    hdrs = ["fake_uart.h"],
    copts = ["-fno-exceptions"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/host:synchronization",
        "@glog",
    ],
)

cc_library(
    name = "console_test_cases",
    testonly = True,
    srcs = ["console_test.cpp"],
    copts = [
        "-Werror",
        "-fno-exceptions",
    ],
    visibility = ["//test:__pkg__"],
    deps = [
        ":uart",
        "@googletest//:gtest",
    ],
    alwayslink = True,
)

cc_test(
    name = "console_test",
    size = "small",
    deps = [
        ":console_test_cases",
        "@googletest//:gtest_main",
    ],
)
