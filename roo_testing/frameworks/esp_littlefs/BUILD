load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "esp_littlefs",
    srcs = ["esp_littlefs.cpp"],
    hdrs = ["include/esp_littlefs.h"],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/microcontrollers/esp32:core",
    ],
)

cc_test(
    name = "esp_littlefs_test",
    size = "small",
    srcs = ["esp_littlefs_test.cpp"],
    deps = [
        ":esp_littlefs",
        "//roo_testing/microcontrollers/esp32:core",
        "@googletest//:gtest_main",
    ],
)
