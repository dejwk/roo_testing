load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "upstream",
    srcs = ["src/FS.cpp"],
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/arduino-esp32/cores/esp32"],
)

alias(
    name = "FS",
    actual = "//roo_testing/frameworks/arduino_host_fs:FS",
    visibility = ["//visibility:public"],
)
