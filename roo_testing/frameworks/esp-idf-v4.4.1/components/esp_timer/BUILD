cc_library(
    name = "esp_timer",
    srcs = [
        "src/esp_timer.c",
        "src/esp_timer_impl_systimer.c",
        "src/system_time.c",
    ],
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]),
    includes = [
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:periph_ctrl",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:systimer_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:esp_newlib",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]),
    includes = [
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
)
