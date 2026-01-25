load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "freertos",
    srcs = glob(
        ["*.c"],
        exclude = ["FreeRTOS-openocd.c"],
    ) + glob(["portable/ThirdParty/GCC/Posix/**/*.c"]) + ["port.cpp"],
    hdrs = glob(["freertos/*.h"]) + glob(["portable/ThirdParty/GCC/Posix/**/*.h"]),
    includes = [
        ".",
        "freertos",
        "portable/ThirdParty/GCC/Posix",
    ],
    visibility = ["//visibility:public"],
    deps = [
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
