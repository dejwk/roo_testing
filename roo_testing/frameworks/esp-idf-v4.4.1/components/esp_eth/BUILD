load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp_eth",
    srcs = ["src/esp_eth.c"],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_timer",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:eth_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/heap",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/tcpip_adapter",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:eth_hal",
    ],
)
