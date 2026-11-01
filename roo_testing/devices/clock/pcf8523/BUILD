load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "pcf8523",
    srcs = ["pcf8523.cpp"],
    hdrs = ["pcf8523.h"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/devices/clock/internal:i2c_rtc"],
)
