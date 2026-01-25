cc_library(
    name = "log",
    srcs = [
        "log.c",
        "log.cpp",
        "log_freertos.c",
    ],
    hdrs = glob(["**/*.h"]),
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/system:timer",
    ],
)

cc_library(
    name = "public",
    hdrs = glob(["**/*.h"]),
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
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
