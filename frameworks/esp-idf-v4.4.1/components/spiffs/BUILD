# cc_library(
#     name = "include",
#     hdrs = glob(["include/**/*.h"]),
#     includes = [
#         "include",
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#     ],
# )

cc_library(
    name = "spiffs",
    srcs = glob(["*.c"]) + [
        "spiffs/src/spiffs_hydrogen.c",
        "spiffs/src/spiffs_cache.c",
    ],
    hdrs = glob(["*.h"]) + glob(["include/**/*.h"]) +
    [
        "spiffs/src/spiffs.h",
        "spiffs/src/spiffs_nucleus.h",
    ],
    includes = [
        ".",
        "include",
        "spiffs/src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        # "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/bootloader_support:include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:gpio",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:spi",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:soc_esp32",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_system:include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_timer",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/hal:spi_flash_hal",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spi_flash",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
    ],
)
