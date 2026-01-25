load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "include",
    hdrs = glob(["include/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "esp_ipc",
    srcs = [
        "src/esp_ipc.c",
        "src/esp_ipc_isr/esp_ipc_isr.c",
    ],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":include",
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ],
)
