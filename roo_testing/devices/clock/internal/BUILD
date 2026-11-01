load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//roo_testing/devices/clock:__subpackages__"])

cc_library(
    name = "registers",
    srcs = ["rtc_registers.cpp"],
    hdrs = ["rtc_registers.h"],
    deps = ["//roo_testing/transducers/time"],
)

cc_library(
    name = "i2c_rtc",
    srcs = ["i2c_rtc.cpp"],
    hdrs = ["i2c_rtc.h"],
    deps = [
        ":registers",
        "//roo_testing/buses/i2c",
    ],
)
