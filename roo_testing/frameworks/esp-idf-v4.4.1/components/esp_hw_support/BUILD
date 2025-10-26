cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
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
    name = "mac_addr",
    srcs = glob(["mac_addr.c"]),
    hdrs = glob(["include/esp_mac.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
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
        "sleep_modes.c"
    ],
    hdrs = [
        "include/esp_sleep.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:rtc_io",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:rtc_io",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:touch_sensor_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
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
        "port/esp32/chip_info.c"
    ],
    hdrs = [
        "include/esp_chip_info.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
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
        "hw_random.cpp"
    ],
    hdrs = [
        "include/esp_random.h"
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
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
    name = "esp_clk",
    srcs = [
        "esp_clk.c",
        "port/esp32/rtc_clk.c"
    ],
    hdrs = [
        "include/soc_log.h",
        "include/esp_private/esp_clk.h",
        "include/soc/esp32/clk.h",
        "include/soc/esp32/rtc.h",
        "include/soc/esp32/dport_access.h",
        "port/esp32/rtc_clk_common.h",
        "port/esp32/regi2c_ctrl.h",
        "port/esp32/private_include/regi2c_apll.h",
        "port/esp32/private_include/regi2c_bbpll.h",
    ],
    includes = [
        "include/soc",
        "include",
        "port/esp32/private_include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:cpu_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
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
