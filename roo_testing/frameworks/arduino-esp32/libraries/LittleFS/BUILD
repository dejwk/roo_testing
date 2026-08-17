load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob(["src/**/*.h"]),
    defines = ["CONFIG_LITTLEFS_PAGE_SIZE=256"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/libraries/FS",
        "//roo_testing/frameworks/esp-idf:sdkconfig",
    ],
)

alias(
    name = "LittleFS",
    actual = "//roo_testing/frameworks/arduino_host_fs:LittleFS",
    visibility = ["//visibility:public"],
)
