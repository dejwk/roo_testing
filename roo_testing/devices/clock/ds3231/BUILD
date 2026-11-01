load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "ds3231",
    srcs = ["ds3231.cpp"],
    hdrs = ["ds3231.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/devices/clock/internal:i2c_rtc",
        "//roo_testing/transducers/temperature",
    ],
)
