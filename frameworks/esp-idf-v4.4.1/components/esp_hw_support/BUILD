cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "soc_spinlock",
    # srcs = glob(["*.cpp"]) + glob(["*.c"]),
    hdrs = glob(["include/soc/spinlock.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "soc_cpu",
    hdrs = glob(["include/soc/cpu.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":esp_cpu"
    ]
)

cc_library(
    name = "esp_cpu",
    hdrs = glob(["include/esp_cpu.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "esp_sleep",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_sleep.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:touch_sensor_hal",
    ]
)

cc_library(
    name = "esp_mac",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_mac.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "esp_chip_info",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_chip_info.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "esp_interface",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_interface.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "esp_random",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_random.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "soc_esp32",
    srcs = [
#        "sleep_modes.c"
    ],
    hdrs = glob([
        "include/soc/esp32/*.h"
    ]),
    includes = [
        "include/soc"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "intr_alloc",
    srcs = ["intr_alloc.c"],
    hdrs = glob(["include/esp_intr_alloc.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/heap",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:interrupt_controller_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ipc",
    ]
)
