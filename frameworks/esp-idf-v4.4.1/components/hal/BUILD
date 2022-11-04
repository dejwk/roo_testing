cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rtc_io_hal",
    srcs = ["rtc_io_hal.c"],
    hdrs = [
        "include/hal/rtc_io_hal.h",
        "include/hal/rtc_io_types.h",
        "include/hal/gpio_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:rtc_io",
    ]
)

cc_library(
    name = "systimer_hal",
    srcs = ["systimer_hal.c"],
    hdrs = [
        "include/hal/systimer_hal.h",
        "include/hal/systimer_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:systimer",
    ]
)

cc_library(
    name = "spi_hal",
    srcs = [
        "spi_hal.c"
    ],
    hdrs = [
        "include/hal/spi_hal.h",
        "include/hal/spi_slave_hal.h",
        "include/hal/spi_types.h",
    ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:spi",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ]
)

cc_library(
    name = "uart_hal",
    srcs = [
        "uart_hal.c",
        "uart_hal_iram.c",
    ],
    hdrs = [
        "include/hal/uart_hal.h",
        "include/hal/uart_types.h",
    ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:uart",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "spi_flash_hal",
    srcs = [
        "spi_flash_hal.c",
        "spi_flash_hal_gpspi.c",
        "spi_flash_hal_iram.c",
    ],
    hdrs = [
        "spi_flash_hal_common.inc",
        "include/hal/spi_flash_hal.h",
        "include/hal/spi_flash_encrypt_hal.h",
        "include/hal/spi_flash_types.h",
    ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:spi_flash",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ]
)

cc_library(
    name = "cpu_hal",
    srcs = ["cpu_hal.c"],
    hdrs = [
        "include/hal/cpu_hal.h",
        "include/hal/cpu_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:cpu",
    ]
)

cc_library(
    name = "touch_sensor_hal",
    srcs = ["touch_sensor_hal.c"],
    hdrs = [
        "include/hal/touch_sensor_hal.h",
        "include/hal/touch_sensor_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:touch_sensor",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "gpio_hal",
    srcs = ["gpio_hal.c"],
    hdrs = [
        "include/hal/gpio_hal.h",
        "include/hal/gpio_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:gpio",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "adc_hal",
    srcs = ["adc_hal.c", "dac_hal.c", "adc_hal.cpp"],
    hdrs = [
        "include/hal/adc_hal.h",
        "include/hal/dac_hal.h",
        "include/hal/adc_types.h",
        "include/hal/dac_types.h",
        "include/hal/dma_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":i2s_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver/esp32:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:adc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:assert",
        "//roo_testing/devices/microcontroller/esp32",
    ]
)

cc_library(
    name = "i2c_hal",
    srcs = ["i2c_hal.c", "i2c_hal_iram.c"],
    hdrs = [
        "include/hal/i2c_hal.h",
        "include/hal/i2c_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:i2c",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "i2s_hal",
    srcs = ["i2s_hal.c"],
    hdrs = [
        "include/hal/i2s_hal.h",
        "include/hal/i2s_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:i2s",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/platform_port/include",
    ]
)

cc_library(
    name = "interrupt_controller_hal_public",
    hdrs = [
        "include/hal/interrupt_controller_hal.h",
        "include/hal/interrupt_controller_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "interrupt_controller_hal",
    srcs = ["interrupt_controller_hal.c"],
    hdrs = [
        "include/hal/interrupt_controller_hal.h",
        "include/hal/interrupt_controller_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:interrupt_controller",
    ]
)

cc_library(
    name = "wdt_hal",
    # srcs = ["wdt_hal.c", "i2c_hal_iram.c"],
    hdrs = [
        "include/hal/wdt_hal.h",
        "include/hal/wdt_types.h",
        ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:wdt",
    ]
)
