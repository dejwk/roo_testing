load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "spi",
    srcs = ["st77xx.cpp"],
    hdrs = ["st77xx.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/spi",
        "//roo_testing/transducers/ui/viewport",
        "@glog",
    ],
)

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "st77xx_test",
    size = "small",
    srcs = ["st77xx_test.cpp"],
    deps = [
        ":spi",
        "@googletest//:gtest_main",
    ],
)
