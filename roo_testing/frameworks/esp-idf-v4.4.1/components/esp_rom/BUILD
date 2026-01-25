cc_library(
    name = "esp_rom",
    # srcs = glob(["**/*.cpp"]) + glob(["src/**/*.c"]),
    srcs = [
        "ets_sys.cpp",
        "gpio.cpp",
        "itoa.c",
        "linux/esp_rom_crc.c",
        "linux/esp_rom_sys.c",
        "reent.cpp",
    ],
    hdrs = glob(["**/*.h"]),
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
        "include/esp32",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:reent",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ],
)

cc_library(
    name = "include",
    # srcs = glob(["**/*.cpp"]) + glob(["src/**/*.c"]),
    hdrs = glob(["**/*.h"]),
    defines = ["CONFIG_IDF_TARGET_ESP32"],
    includes = [
        "include",
        "include/esp32",
    ],
    visibility = ["//visibility:public"],
    deps = [
        # "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32:include",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:reent",
    ],
)
