cc_library(
    name = "arduino_main",
    srcs = glob(["cores/esp32/main.cpp"]),
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino",
    srcs = glob(["cores/esp32/*hal*.c"]) + glob(
        ["**/*.cpp"],
        exclude = ["cores/esp32/main.cpp"],
    ) + glob([
        "tools/sdk/src/*.c",
    ]) + glob([
        "tools/sdk/include/esp32/*.h",
    ]),
    hdrs = glob(["**/*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    includes = [
        "cores/esp32/",
        "libraries/FS",
        "libraries/SD/src",
        "libraries/SPI/src",
        "libraries/SPIFFS",
        "libraries/WiFi/src",
        "libraries/Wire/src",
        "tools/sdk/include/config",
        "tools/sdk/include/driver",
        "tools/sdk/include/driver/driver",
        "tools/sdk/include/esp32",
        "tools/sdk/include/ethernet",
        "tools/sdk/include/fatfs",
        "tools/sdk/include/log",
        "tools/sdk/include/sdmmc",
        "tools/sdk/include/soc",
        "tools/sdk/include/soc/soc",
        "tools/sdk/include/spi_flash",
        "tools/sdk/include/spiffs",
        "tools/sdk/include/tcpip_adapter",
        "tools/sdk/include/vfs",
        "tools/sdk/include/wear_levelling",
        "variants/esp32",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/i2c",
        "//roo_testing/buses/gpio",
        "//roo_testing/buses/spi",
        "//roo_testing/transducers/time",
        "//roo_testing/transducers/time:default_system_clock",
        "@com_github_google_glog//:glog",
    ],
)
