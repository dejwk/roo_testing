load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "spi",
    srcs = ["ili9488spi.cpp"],
    hdrs = ["ili9488spi.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/spi",
        "//roo_testing/transducers/ui/viewport",
        "@glog",
    ],
)
