load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp32",
    defines = ["CORE_DEBUG_LEVEL=5"],
    includes = [
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":core",
    ],
)

cc_library(
    name = "core",
    srcs = glob(["*.cpp"], exclude = ["*_test.cpp"]),
    hdrs = glob(["*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/esp_now",
        "//roo_testing/buses/gpio",
        "//roo_testing/buses/i2c",
        "//roo_testing/buses/onewire",
        "//roo_testing/buses/spi",
        "//roo_testing/buses/uart",
        "//roo_testing/sys",
        "//roo_testing/system:timer",
        "//roo_testing/transducers/time",
        "//roo_testing/transducers/wifi",
        "@glog",
    ],
)

cc_library(
    name = "nvs_storage",
    srcs = ["fake_esp32_nvs.cpp"],
    hdrs = ["fake_esp32_nvs.h"],
    visibility = ["//visibility:public"],
)
