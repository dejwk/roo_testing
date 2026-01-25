load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

proto_library(
    name = "nvs_proto",
    srcs = ["storage.proto"],
)

cc_proto_library(
    name = "nvs_cc_proto",
    deps = [":nvs_proto"],
)

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
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":nvs_cc_proto",
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
