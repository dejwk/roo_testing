load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "freertos",
    srcs = glob(
        ["*.c"],
        exclude = ["FreeRTOS-openocd.c"],
    ) + ["port.cpp"],
    hdrs = glob(["freertos/*.h"]) + glob(["portable/ThirdParty/GCC/Posix/**/*.h"]),
    includes = [
        ".",
        "freertos",
        "portable/ThirdParty/GCC/Posix",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":freertos_posix_port",
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_spinlock",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "freertos_posix_port",
    srcs = glob(["portable/ThirdParty/GCC/Posix/**/*.c"]),
    hdrs = glob(["portable/ThirdParty/GCC/Posix/**/*.h"]),
    copts = [
        # Avoid libunwind/forced-unwind crashes in the POSIX FreeRTOS port.
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
    ],
    includes = [
        ".",
        "freertos",
        "portable/ThirdParty/GCC/Posix",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":include",
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_spinlock",
        "//roo_testing/system:timer",
        "@glog",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["freertos/*.h"]) + glob(["portable/ThirdParty/GCC/Posix/**/*.h"]),
    includes = [
        ".",
        "freertos",
        "portable/ThirdParty/GCC/Posix",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:include",
        "//roo_testing/system:include",
    ],
)
