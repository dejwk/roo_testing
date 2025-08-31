cc_library(
    name = "esp_wifi",
    srcs = [
        "esp_wifi.cpp",
        "esp_now.cpp",
        "esp_netif_linux.c",
        "src/smartconfig.c",
        "src/wifi_netif.c",
        "src/wifi_init.c",
        "src/wifi_default.c",
    ],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_event",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_interface",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_netif",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/tcpip_adapter",
        "//roo_testing/devices/microcontroller/esp32",
        "//roo_testing/sys",
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
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_event:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:include",
    ]
)
