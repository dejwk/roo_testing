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

# Recompile the real adapter with a tiny epoch space so exhaustion is reachable
# through the public ESP-IDF API in one isolated test process.
cc_library(
    name = "interrupts_epoch_test_support",
    srcs = ["esp_interrupts.cpp"],
    hdrs = ["esp_interrupts.h"],
    copts = ["-Werror"],
    local_defines = [
        "ROO_TESTING_INTERNAL_ESP_INTERRUPT_MAX_EPOCH=2",
    ],
    testonly = True,
    visibility = ["//test:__pkg__"],
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
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/frameworks/esp-idf/components/freertos",
    ],
    alwayslink = 1,
)
