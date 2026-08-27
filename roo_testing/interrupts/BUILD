load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "interrupt_controller",
    srcs = ["interrupt_controller.cpp"],
    hdrs = ["interrupt_controller.h"],
    copts = ["-Werror"],
    include_prefix = "roo_testing/interrupts",
    strip_include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp-idf/components/freertos:roo_testing_port_api",
    ],
)
