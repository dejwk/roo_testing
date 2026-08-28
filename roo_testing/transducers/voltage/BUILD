load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "voltage",
    srcs = [
        "voltage.cpp",
        "voltage_signal.cpp",
    ],
    hdrs = [
        "voltage.h",
        "voltage_signal.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/host:synchronization",
        "//roo_testing/transducers",
        "//roo_testing/system:timer",
        "@glog",
    ],
)
