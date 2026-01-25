load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "spi",
    hdrs = ["xpt2046spi.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/spi",
        "//roo_testing/transducers/ui/viewport",
        "@glog",
    ],
)
