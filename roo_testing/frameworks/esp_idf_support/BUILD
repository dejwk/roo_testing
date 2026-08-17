load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Host process entry point for an ESP-IDF-style application. The source is
# destination-owned because upstream ESP-IDF startup is target/bootloader
# specific and cannot serve as a native Linux main().
cc_library(
    name = "main",
    srcs = ["main.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/environment:idf",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
    alwayslink = 1,
)
