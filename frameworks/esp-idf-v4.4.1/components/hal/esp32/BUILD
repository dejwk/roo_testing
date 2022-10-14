# cc_library(
#     name = "esp32",
#     srcs = glob(["**/*.cpp"]) + glob(["**/*.c"]),
#     hdrs = glob(["**/*.h"]),
#     includes = [
#         "include"
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#         "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32/include",
#         #"//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/include",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
#         "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
#     ]
# )

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
        # "platform_port/include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rtc_io",
    # srcs = glob(["**/*.cpp"]) + glob(["**/*.c"]),
    hdrs = ["include/hal/rtc_io_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "cpu",
    hdrs = ["include/hal/cpu_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "touch_sensor",
    hdrs = ["include/hal/touch_sensor_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "gpio",
    srcs = ["gpio.cpp"],
    hdrs = ["include/hal/gpio_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "adc",
    srcs = [
        "adc_hal.c",
    ],
    hdrs = [
        "include/hal/adc_hal.h",
        "include/hal/adc_hal_conf.h",
        "include/hal/adc_ll.h",
        "include/hal/dac_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:include",
        ":i2s",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "i2s",
    hdrs = ["include/hal/i2s_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "interrupt_controller",
    srcs = ["interrupt_descriptor_table.c"],
    hdrs = ["include/hal/interrupt_controller_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:interrupt_controller_hal_public",
    ]
)
