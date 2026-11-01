load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Destination-owned overlay: expose native host declarations. Device register
# headers and the upstream hardware/stdio stub implementations are not compiled.
cc_library(
    name = "headers",
    hdrs = glob([
        "src/common/pico_base_headers/include/**/*.h",
        "src/common/pico_time/include/**/*.h",
        "src/host/pico_platform/include/**/*.h",
        "src/host/hardware_timer/include/**/*.h",
        "src/host/hardware_gpio/include/**/*.h",
        "src/host/hardware_irq/include/**/*.h",
        "src/host/hardware_uart/include/**/*.h",
        "src/boards/include/**/*.h",
        "roo_testing_host_include/**/*.h",
    ]),
    includes = [
        "roo_testing_host_include",
        "src/boards/include",
        "src/common/pico_base_headers/include",
        "src/common/pico_time/include",
        "src/host/hardware_gpio/include",
        "src/host/hardware_irq/include",
        "src/host/hardware_timer/include",
        "src/host/hardware_uart/include",
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
        "//roo_testing/frameworks/pico_support:gpio",
        "//roo_testing/frameworks/pico_support:time",
        "//roo_testing/frameworks/pico_support:uart",
    ],
)
