load("@rules_proto//proto:defs.bzl", "proto_library")

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
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        ":core",
    ],
)

cc_library(
    name = "core",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/buses/esp_now",
        "//roo_testing/buses/i2c",
        "//roo_testing/buses/gpio",
        "//roo_testing/buses/uart",
        "//roo_testing/buses/spi",
        "//roo_testing/buses/onewire",
        "//roo_testing/transducers/time",
        "//roo_testing/transducers/wifi",
        "//roo_testing/system:timer",
        "//roo_testing/sys",
        "@glog//:glog",
        ":nvs_cc_proto"
    ],
)
