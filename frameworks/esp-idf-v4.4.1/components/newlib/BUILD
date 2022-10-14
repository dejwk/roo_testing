# cc_library(
#     name = "newlib",
#     srcs = glob(["**/*.cpp"]) + glob(["*.c"]),
#     hdrs = glob(["**/*.h"]),
#     includes = [
#         "include",
#         "platform_include"
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#         "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/log:public",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/heap/include",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal",
#     ]
# )

cc_library(
    name = "locks",
    srcs = ["locks.c"],
    hdrs = [
        "platform_include/sys/lock.h",
        "platform_include/sys/reent.h",
        ],
    includes = [
        # "include",
        "platform_include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/log:public",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_cpu",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_cpu",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_spinlock",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/heap/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom/esp32",
    ]
)

cc_library(
    name = "assert",
    srcs = ["assert.c"],
    hdrs = [
        "platform_include/assert.h",
        ],
    includes = [
        # "include",
        "platform_include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/log:public",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_cpu",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_cpu",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_spinlock",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/heap/include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom/esp32",
    ]
)
