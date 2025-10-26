cc_library(
    name = "nvs_flash",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob(["include/**/*.h"]) + glob(["include/**/*.hpp"]) + glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    defines = [
        "ESP_PLATFORM",
    ],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spi_flash",
        "//roo_testing/devices/microcontroller/esp32",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]) + glob(["include/**/*.hpp"]) + glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)
