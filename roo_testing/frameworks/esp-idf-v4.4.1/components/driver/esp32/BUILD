load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "adc",
    srcs = [
        "adc.c",
        "dac.c",
    ],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:rtc_io",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:adc_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:adc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ],
)
