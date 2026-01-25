load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "gpio",
    srcs = ["fake_gpio.cpp"],
    hdrs = ["fake_gpio.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/transducers/voltage",
        "@glog",
    ],
)
