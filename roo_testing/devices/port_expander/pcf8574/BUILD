load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "pcf8574",
    srcs = ["pcf8574.cpp"],
    hdrs = ["pcf8574.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/gpio",
        "//roo_testing/buses/i2c",
        "@glog",
    ],
)
