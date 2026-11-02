load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "kernel_test",
    size = "small",
    timeout = "short",
    srcs = ["kernel_test.cpp"],
    linkstatic = True,
    deps = ["//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos"],
)
