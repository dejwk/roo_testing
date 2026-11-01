load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ds3234",
    srcs = ["ds3234.cpp"],
    hdrs = ["ds3234.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/spi",
        "//roo_testing/devices/clock/internal:registers",
    ],
)
