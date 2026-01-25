load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "voltage",
    srcs = ["voltage.cpp"],
    hdrs = ["voltage.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/transducers",
        "@glog",
    ],
)
