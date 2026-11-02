load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# The runtime boundary keeps the independently pinned kernels out of each
# other's configured dependency graphs.
cc_library(
    name = "freertos_headers",
    deps = ["//roo_testing/frameworks/environment:freertos"] + select({
        "//roo_testing/platforms:is_arduino_rp2040_freertos": ["//roo_testing/frameworks/freertos_posix:compat_headers"],
        "//conditions:default": ["//roo_testing/frameworks/esp-idf/components/freertos"],
    }),
)

cc_library(
    name = "freertos",
    deps = [":freertos_headers"] + select({
        "//roo_testing/platforms:is_arduino_rp2040_freertos": ["//roo_testing/frameworks/arduino-pico/FreeRTOS-Kernel:freertos"],
        "//conditions:default": ["//roo_testing/frameworks/esp-idf/components/freertos"],
    }),
)
