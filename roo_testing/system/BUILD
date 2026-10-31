load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "timer",
    srcs = ["timer.cpp"],
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/host:synchronization",
        "@glog",
    ],
)

cc_library(
    name = "manual_time_mode",
    srcs = ["manual_time_mode.cpp"],
    visibility = ["//visibility:public"],
    alwayslink = 1,
)

cc_library(
    name = "include",
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "wall_clock",
    srcs = ["wall_clock.cpp"],
    hdrs = ["wall_clock.h"],
    copts = ["-fno-exceptions"],
    visibility = ["//visibility:public"],
    deps = [
        ":timer",
        "//roo_testing/host:synchronization",
    ],
)
