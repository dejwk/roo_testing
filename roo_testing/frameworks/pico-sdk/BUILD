load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Destination-owned overlay: import only native host header roots. Device
# register headers and unimplemented host GPIO/UART stubs are not exported.
cc_library(
    name = "headers",
    hdrs = glob([
        "src/common/pico_base_headers/include/**/*.h",
        "src/common/pico_time/include/**/*.h",
        "src/host/pico_platform/include/**/*.h",
        "src/host/hardware_timer/include/**/*.h",
        "src/boards/include/**/*.h",
        "roo_testing_host_include/**/*.h",
    ]),
    includes = [
        "roo_testing_host_include",
        "src/boards/include",
        "src/common/pico_base_headers/include",
        "src/common/pico_time/include",
        "src/host/hardware_timer/include",
        "src/host/pico_platform/include",
    ],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/frameworks/environment:pico_sdk"],
)

cc_library(
    name = "pico_sdk",
    srcs = [
        "src/host/pico_platform/platform_base.c",
    ],
    linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        ":headers",
        "//roo_testing/frameworks/pico_support:time",
    ],
)
