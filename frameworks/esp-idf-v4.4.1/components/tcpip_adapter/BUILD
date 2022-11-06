cc_library(
    name = "tcpip_adapter",
    srcs = ["tcpip_adapter_compat.c"],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_netif:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/lwip:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/lwip/lwip/src:include",
    ],
)

# cc_library(
#     name = "include",
#     hdrs = glob(["include/**/*.h"]),
#     includes = [
#         "include",
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/lwip:include",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/lwip/lwip/src:include",
#     ],
# )
