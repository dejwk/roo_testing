cc_library(
    name = "log",
    srcs = ["log_freertos.c", "log.cpp", "log.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/devices/microcontroller/esp32",
    ]
)

cc_library(
    name = "public",
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    # deps = [
    #     "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    #     # "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
    #     # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support",
    # ]
)
