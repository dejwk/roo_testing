load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp_adc_cal",
    srcs = [
        "esp32/esp_adc_cal.c",
        "esp_adc_cal_common.c",
    ],
    hdrs = [
        "esp_adc_cal_internal.h",
        "include/esp_adc_cal.h",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:adc_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ],
)

cc_library(
    name = "include",
    hdrs = [
        "esp_adc_cal_internal.h",
        "include/esp_adc_cal.h",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    # deps = [
    #     "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:include",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:adc_hal",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
    #     "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    # ],
)
