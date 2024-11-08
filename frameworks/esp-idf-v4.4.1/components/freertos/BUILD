cc_library(
    name = "freertos",
    srcs = glob(["*.c"], exclude = ["FreeRTOS-openocd.c"]) + glob(["portable/ThirdParty/GCC/Posix/**/*.c"]) + ["port.cpp"],
    hdrs = glob(["freertos/*.h"]) + glob(["portable/ThirdParty/GCC/Posix/**/*.h"]),
    includes = [
        ".",
        "freertos",
        "portable/ThirdParty/GCC/Posix",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_google_glog//:glog",
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_cpu",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_cpu",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_spinlock",
    ],
)