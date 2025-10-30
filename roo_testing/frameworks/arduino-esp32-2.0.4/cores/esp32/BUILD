cc_library(
    name = "esp32",
    srcs = 
        glob(["**/*.c"]) +
        glob(["**/*.cpp"], exclude = ["main.cpp", "gtest_main.cpp"]) +
        glob(["**/*.h"]),
    hdrs = glob(["**/*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    includes = ["."],
    linkstatic = 1,
    deps = [
        "//roo_testing/microcontrollers/esp32",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/tools/sdk/esp32",
#        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
#        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
#        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "arduino",
    visibility = ["//visibility:public"],
    deps = [
        ":esp32",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/FS",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/Preferences",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/SD",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/SD_MMC",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/SPI",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/SPIFFS",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/WiFi",
        "//roo_testing/frameworks/arduino-esp32-2.0.4/libraries/Wire",
    ]
)

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    alwayslink = 1,
    linkstatic = 1,
    deps = [
        ":esp32",
        ":arduino",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["gtest_main.cpp"],
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    alwayslink = 1,
    linkstatic = 1,
    deps = [
        ":esp32",
        ":arduino",
        "@googletest//:gtest",
    ]
)
