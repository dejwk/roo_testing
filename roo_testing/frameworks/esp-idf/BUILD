load("@rules_cc//cc:cc_library.bzl", "cc_library")

# ESP-IDF's target-oriented component build graph is intentionally not mirrored.
# Host builds consume current public headers and the upstream Linux FreeRTOS
# port, while hardware-facing symbols are provided by //.../esp32_shims.
cc_library(
    name = "freertos_support_headers",
    hdrs = ["sdkconfig.h"] + glob([
        "components/esp_common/include/**/*.h",
        "components/esp_system/include/**/*.h",
        "components/heap/include/**/*.h",
    ]),
    includes = [
        ".",
        "components/esp_common/include",
        "components/esp_system/include",
        "components/heap/include",
    ],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/soc:target"],
)

cc_library(
    name = "headers",
    hdrs = glob([
        "components/**/*.h",
        "components/**/*.inc",
    ]),
    defines = [
        "IDF_VER=\\\"v6.0.2\\\"",
    ],
    includes = [
        ".",
        "components/app_update/include",
        "components/bootloader_support/include",
        "components/bt/include",
        "components/bt/include/esp32/include",
        "components/driver/i2c/include",
        "components/efuse/include",
        "components/efuse/esp32/include",
        "components/esp_adc/include",
        "components/esp_adc/esp32/include",
        "components/esp_app_format/include",
        "components/esp_bootloader_format/include",
        "components/esp_coex/include",
        "components/esp_common/include",
        "components/esp_driver_gpio/include",
        "components/esp_driver_gptimer/include",
        "components/esp_driver_i2c/include",
        "components/esp_driver_i2s/include",
        "components/esp_driver_ledc/include",
        "components/esp_driver_rmt/include",
        "components/esp_driver_sdm/include",
        "components/esp_driver_sdmmc/include",
        "components/esp_driver_sdspi/include",
        "components/esp_driver_spi/include",
        "components/esp_driver_touch_sens/include",
        "components/esp_driver_uart/include",
        "components/esp_eth/include",
        "components/esp_event/include",
        "components/esp_hal_gpio/include",
        "components/esp_hal_gpio/esp32/include",
        "components/esp_hw_support/include",
        "components/esp_mm/include",
        "components/esp_netif/include",
        "components/esp_partition/include",
        "components/esp_phy/include",
        "components/esp_phy/esp32/include",
        "components/esp_psram/include",
        "components/esp_ringbuf/include",
        "components/esp_rom/include",
        "components/esp_rom/esp32/include",
        "components/esp_rom/esp32/include/esp32",
        "components/esp_system/include",
        "components/esp_system/port/include",
        "components/esp_timer/include",
        "components/esp_wifi/include",
        "components/esp_wifi/include/local",
        "components/hal/include",
        "components/hal/esp32/include",
        "components/hal/linux/include",
        "components/hal/platform_port/include",
        "components/heap/include",
        "components/log/include",
        "components/lwip/include",
        "components/lwip/lwip/src/include",
        "components/lwip/port/include",
        "components/lwip/port/linux/include",
        "components/lwip/port/freertos/include",
        "components/nvs_flash/include",
        "components/pthread/include",
        "components/sdmmc/include",
        "components/soc/include",
        "components/soc/esp32/include",
        "components/soc/esp32/register",
        "components/soc/linux/include",
        "components/spi_flash/include",
        "components/spiffs/include",
        "components/vfs/include",
        "components/wear_levelling/include",
        "components/wpa_supplicant/include",
        "components/wpa_supplicant/port/include",
        "components/xtensa/include",
        "components/xtensa/esp32/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":linux_machine_headers",
        ":sdkconfig",
        "//roo_testing/frameworks/esp-idf/components/freertos:include",
    ],
)

# Only the Linux machine byte-order shim is portable to glibc. Exporting the
# whole upstream components/linux/include directory would also shadow glibc's
# sys/cdefs.h with the BSD compatibility version.
cc_library(
    name = "linux_machine_headers",
    hdrs = ["components/linux/include/machine/endian.h"],
    strip_include_prefix = "components/linux/include",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "core",
    visibility = ["//visibility:public"],
    deps = [
        ":headers",
        "//roo_testing/frameworks/esp-idf/components/freertos",
        "//roo_testing/frameworks/esp32_shims:idf",
    ],
)

alias(
    name = "esp-idf",
    actual = ":core",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sdkconfig",
    hdrs = ["sdkconfig.h"],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = ["//roo_testing/soc:target"],
)
