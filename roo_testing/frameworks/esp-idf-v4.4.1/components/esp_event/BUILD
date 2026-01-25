load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp_event",
    srcs = [
        "default_event_loop.c",
        "esp_event.c",
    ],
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]) + ["event_send_compat.inc"],
    includes = [
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_netif:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
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
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_netif:include",
    ],
)
