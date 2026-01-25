cc_library(
    name = "esp_netif",
    srcs = [
        "esp_netif_objects.c",
        #        "lwip/esp_netif_lwip.c",
    ],
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]) + glob(["lwip/**/*.h"]),
    includes = [
        "include",
        "lwip",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_eth:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include",  # for esp_wifi_types.h
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/tcpip_adapter",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]),
    includes = [
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_eth:include",
        #        "//roo_testing/frameworks/esp-idf-v4.4.1/components/tcpip_adapter:include",
        #        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_wifi:include", # for esp_wifi_types.h
    ],
)
