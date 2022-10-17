cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
    ]
)

cc_library(
    name = "rtc_io",
    srcs = ["rtc_io.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
    ]
)

cc_library(
    name = "gpio",
    srcs = ["gpio.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
    ]
)

cc_library(
    name = "i2c",
    srcs = ["i2c.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_pm/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:i2c_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
    ]
)

cc_library(
    name = "adc",
    srcs = ["adc.c", "adc_common.c", "dac_common.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver/esp32:adc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_pm/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:adc_hal",
#        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:dma_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
    ]
)

cc_library(
    name = "i2s",
    srcs = ["i2s.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_pm/include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:adc_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:i2s_hal",
    ]
)

cc_library(
    name = "periph_ctrl",
    srcs = ["periph_ctrl.c"],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include"
    ],
    visibility = ["//visibility:public"],
    deps = [
        # "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal/esp32:clk_gate",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:intr_alloc",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_pm/include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:i2c_hal",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:gpio_hal",
        # # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:rtc_io_hal",
    ]
)
