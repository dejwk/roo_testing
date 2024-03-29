cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rtc_io",
    hdrs = [
        "include/hal/rtc_io_ll.h",
        "include/hal/rtc_cntl_ll.h"
    ],
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

# cc_library(
#     name = "systimer",
#     hdrs = ["include/hal/systimer_ll.h"],
#     includes = [
#         "include"
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#     ]
# )

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
    name = "spi",
    hdrs = [
        "include/hal/spi_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        ":gpio",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "ledc",
    hdrs = [
        "include/hal/ledc_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = []
)

cc_library(
    name = "timer",
    hdrs = [
        "include/hal/timer_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = []
)

cc_library(
    name = "spi_flash",
    hdrs = [
        "include/hal/spi_flash_ll.h",
        "include/hal/spi_flash_encrypted_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        ":spi",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        # ":gpio",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "i2c",
    srcs = ["i2c.cpp"],
    hdrs = ["include/hal/i2c_ll.h"],
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

cc_library(
    name = "clk_gate",
    # srcs = ["interrupt_descriptor_table.c"],
    hdrs = ["include/hal/clk_gate_ll.h"],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_esp32",
    ]
)

cc_library(
    name = "wdt",
    hdrs = [
        "include/hal/mwdt_ll.h",
        "include/hal/rwdt_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "uart",
    hdrs = [
        "include/hal/uart_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)

cc_library(
    name = "soc",
    hdrs = [
        "include/hal/soc_ll.h",
    ],
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
    ]
)
