cc_library(
    name = "esp_rom",
    # srcs = glob(["**/*.cpp"]) + glob(["src/**/*.c"]),
    srcs = [
        "ets_sys.cpp", 
        "gpio.cpp",
        "reent.cpp",
        "itoa.c",
        "linux/esp_rom_sys.c",
        "linux/esp_rom_crc.c",
    ],
    hdrs = glob(["**/*.h"]),
    includes = [
        "include",
        "include/esp32"
    ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:reent",
    ]
)

cc_library(
    name = "include",
    # srcs = glob(["**/*.cpp"]) + glob(["src/**/*.c"]),
    hdrs = glob(["**/*.h"]),
    includes = [
        "include",
        "include/esp32"
    ],
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    visibility = ["//visibility:public"],
    deps = [
        # "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32:include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:reent",
    ]
)
