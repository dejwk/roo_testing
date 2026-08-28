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

cc_library(
    name = "interrupt_controller_exhaustion_test_support",
    srcs = ["interrupt_controller.cpp"],
    hdrs = ["interrupt_controller.h"],
    copts = ["-Werror"],
    include_prefix = "roo_testing/interrupts",
    local_defines = ["ROO_TESTING_INTERNAL_INTERRUPT_MAX_GENERATION=2"],
    strip_include_prefix = ".",
    testonly = True,
    visibility = ["//test:__pkg__"],
    deps = [
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp-idf/components/freertos:roo_testing_port_api",
    ],
)
