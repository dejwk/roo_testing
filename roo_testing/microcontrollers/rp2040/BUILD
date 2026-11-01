load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "pico",
    srcs = ["fake_pico.cpp"],
    hdrs = ["fake_pico.h"],
    copts = ["-fno-exceptions"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/gpio",
        "//roo_testing/buses/spi",
        "//roo_testing/buses/uart",
        "//roo_testing/frameworks/pico-sdk:headers",
        "@glog",
    ],
)
