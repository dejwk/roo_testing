load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "headers",
    hdrs = glob(["**/*.h"]),
    defines = [
        "ARDUINO=10819",
        "ARDUINO_ARCH_ESP32",
        "ARDUINO_USB_CDC_ON_BOOT=0",
        "ARDUINO_USB_DFU_ON_BOOT=0",
        "ARDUINO_USB_MSC_ON_BOOT=0",
        "ARDUINO_USB_MODE=0",
        "ARDUINO_USB_ON_BOOT=0",
        "CORE_DEBUG_LEVEL=5",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32/variants:esp32",
        "//roo_testing/frameworks/esp-idf:headers",
    ],
)

# Portable sources come from current upstream. Translation units that directly
# access ESP32 registers are replaced by the host shim package.
cc_library(
    name = "esp32",
    srcs = [
        "ColorFormat.c",
        "FunctionalInterrupt.cpp",
        "HEXBuilder.cpp",
        "HardwareSerial.cpp",
        "IPAddress.cpp",
        "MacAddress.cpp",
        "Print.cpp",
        "Stream.cpp",
        "StreamString.cpp",
        "StringUtils.cpp",
        "WMath.cpp",
        "WString.cpp",
        "base64.cpp",
        "cbuf.cpp",
        "libb64/cdecode.c",
        "libb64/cencode.c",
        "stdlib_noniso.c",
        "wiring_pulse.c",
        "wiring_shift.c",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":headers",
        "//roo_testing/frameworks/esp-idf",
        "//roo_testing/frameworks/esp32_shims:arduino",
        "//roo_testing/frameworks/esp32_shims:idf",
        "//roo_testing/microcontrollers/esp32",
    ],
)

cc_library(
    name = "arduino",
    visibility = ["//visibility:public"],
    deps = [
        ":esp32",
        "//roo_testing/frameworks/arduino-esp32/libraries/FS",
        "//roo_testing/frameworks/arduino-esp32/libraries/Preferences",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD",
        "//roo_testing/frameworks/arduino-esp32/libraries/SD_MMC",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPI",
        "//roo_testing/frameworks/arduino-esp32/libraries/SPIFFS",
        "//roo_testing/frameworks/arduino-esp32/libraries/WiFi",
        "//roo_testing/frameworks/arduino-esp32/libraries/Wire",
    ],
)

alias(
    name = "main",
    actual = "//roo_testing/frameworks/arduino_support:main",
    visibility = ["//visibility:public"],
)

alias(
    name = "gtest_main",
    actual = "//roo_testing/frameworks/arduino_support:gtest_main",
    visibility = ["//visibility:public"],
)
