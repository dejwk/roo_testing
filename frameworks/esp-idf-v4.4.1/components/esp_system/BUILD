cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_chip_info",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_random",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_mac",
    ]
)

cc_library(
    name = "esp_system",
    hdrs = glob(["**/*.h"]),
    srcs = glob(["*.c"]) + glob(["port/*.c"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    defines = [
        "IDF_VER=\\\"v4.4.1-472-gc9140caf8c\\\"",
    ],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_chip_info",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_mac",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_random",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:uart_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:touch_sensor_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:wdt_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:wdt",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/nvs_flash",
    ]
)
