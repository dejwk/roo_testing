cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "spi_flash",
    srcs = glob(["*.c"]),
    hdrs = glob(["*.h"]) + glob(["include/**/*.h"]),
    includes = [
        ".",
        "include",
        "include/spi_flash",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/bootloader_support:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:gpio",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:spi",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_timer",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:spi_flash_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ],
)
