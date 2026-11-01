load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ds1307",
    srcs = ["ds1307.cpp"],
    hdrs = ["ds1307.h"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/devices/clock/internal:i2c_rtc"],
)
