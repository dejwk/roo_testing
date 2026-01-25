load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "wifi",
    srcs = ["wifi.cpp"],
    hdrs = ["wifi.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/transducers",
        "@glog",
    ],
)
