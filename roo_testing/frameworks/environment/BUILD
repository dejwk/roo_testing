load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Build identities are exported as normal C++ providers instead of workspace
# --copt flags. This makes the compile context follow the Bazel dependency that
# actually provides the framework, including when roo_testing is an external
# module.
cc_library(
    name = "emulator",
    defines = ["ROO_TESTING=1"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "esp_idf",
    defines = [
        "ESP_PLATFORM=1",
        "IDF_VER=\\\"v6.0.2\\\"",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":emulator",
        "//roo_testing/soc:target",
    ],
)

cc_library(
    name = "arduino",
    defines = [
        "ARDUINO=10819",
        "ARDUINO_ARCH_ESP32=1",
        "ARDUINO_BOARD=\\\"ESP32_DEV\\\"",
        "ARDUINO_ESP32_DEV=1",
        "ARDUINO_USB_CDC_ON_BOOT=0",
        "ARDUINO_USB_DFU_ON_BOOT=0",
        "ARDUINO_USB_MSC_ON_BOOT=0",
        "ARDUINO_USB_MODE=0",
        "ARDUINO_USB_ON_BOOT=0",
        "ARDUINO_VARIANT=\\\"esp32\\\"",
        "CORE_DEBUG_LEVEL=5",
    ],
    visibility = ["//visibility:public"],
    deps = [":esp_idf"],
)
