cc_library(
    name = "wifi_provisioning",
    srcs = [
        "src/manager.c",
    ],
    hdrs = glob(["include/**/*.h"]) + [
        "src/wifi_provisioning_priv.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/protocomm:include",
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
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/protocomm:include",
    ],
)
