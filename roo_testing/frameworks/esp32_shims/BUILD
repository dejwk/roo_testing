load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "esp32_shims",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    defines = [
        "ARDUINO_ARCH_ESP32",
        "CONFIG_IDF_TARGET_ESP32",
        "ESP_PLATFORM",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf:headers",
        "//roo_testing/frameworks/esp-idf/components/freertos:freertos",
        "//roo_testing/microcontrollers/esp32:core",
        "//roo_testing/sys",
        "//roo_testing/system:timer",
        "@glog",
    ],
)
