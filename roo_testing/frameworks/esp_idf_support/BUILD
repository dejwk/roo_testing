load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "interrupts",
    srcs = ["esp_interrupts.cpp"],
    hdrs = ["esp_interrupts.h"],
    copts = ["-Werror"],
    include_prefix = "roo_testing/frameworks/esp_idf_support",
    strip_include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/interrupts:interrupt_controller",
    ],
)

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
