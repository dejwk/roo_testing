load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "spi",
    srcs = ["ssd1327spi.cpp"],
    hdrs = ["ssd1327spi.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/spi",
        "//roo_testing/transducers/ui/viewport",
    ],
)
