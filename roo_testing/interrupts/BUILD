load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "interrupt_controller",
    srcs = [
        "interrupt_controller.cpp",
    ] + select({
        "//roo_testing/platforms:is_bare_metal": ["pico_interrupt_delivery.cpp"],
        "//conditions:default": ["freertos_interrupt_delivery.cpp"],
    }),
    hdrs = [
        "interrupt_controller.h",
        "interrupt_delivery.h",
        "pico_interrupt_delivery.h",
    ],
    copts = ["-Werror"],
    include_prefix = "roo_testing/interrupts",
    strip_include_prefix = ".",
    visibility = ["//visibility:public"],
    deps = select({
        "//roo_testing/platforms:is_bare_metal": [],
        "//conditions:default": [
            "//roo_testing/frameworks/esp-idf/components/freertos",
            "//roo_testing/frameworks/esp-idf/components/freertos:roo_testing_port_api",
        ],
    }),
)

cc_library(
    name = "interrupt_controller_exhaustion_test_support",
    srcs = [
        "freertos_interrupt_delivery.cpp",
        "interrupt_controller.cpp",
        "interrupt_delivery.h",
    ],
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
