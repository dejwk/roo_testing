load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/arduino-esp32/libraries/FS"],
)

alias(
    name = "SPIFFS",
    actual = "//roo_testing/frameworks/arduino_host_fs:SPIFFS",
    visibility = ["//visibility:public"],
)
