load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "sd_spi",
    srcs = ["sd_spi.cpp"],
    hdrs = ["sd_spi.h"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/buses/spi"],
)

cc_test(
    name = "sd_spi_test",
    size = "small",
    srcs = ["sd_spi_test.cpp"],
    deps = [
        ":sd_spi",
        "@googletest//:gtest_main",
    ],
)
