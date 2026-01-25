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
