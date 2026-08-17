load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = [
        "src/SD.h",
        "src/sd_defines.h",
        "src/sd_diskio.h",
    ],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPI",
    ],
)

alias(
    name = "SD",
    actual = "//roo_testing/frameworks/arduino_host_fs:SD",
    visibility = ["//visibility:public"],
)
