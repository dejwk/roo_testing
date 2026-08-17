load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/FS",
        "//roo_testing/frameworks/esp-idf:headers",
    ],
)

alias(
    name = "SD_MMC",
    actual = "//roo_testing/frameworks/arduino_host_fs:SD_MMC",
    visibility = ["//visibility:public"],
)
