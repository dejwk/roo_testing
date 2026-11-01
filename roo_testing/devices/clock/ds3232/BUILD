load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ds3232",
    srcs = ["ds3232.cpp"],
    hdrs = ["ds3232.h"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/devices/clock/internal:i2c_rtc"],
)
