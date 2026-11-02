load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "config",
    hdrs = [
        "FreeRTOSConfig.h",
        "freertos_tasks_c_additions.h",
        "runtime.h",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "hooks",
    srcs = ["hooks.cpp"],
    hdrs = ["runtime.h"],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:headers",
        "//roo_testing/system:timer",
    ],
    alwayslink = True,
)
