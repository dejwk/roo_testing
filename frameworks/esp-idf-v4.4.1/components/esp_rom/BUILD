cc_library(
    name = "esp_rom",
    # srcs = glob(["**/*.cpp"]) + glob(["src/**/*.c"]),
    srcs = [
        "ets_sys.cpp", 
        "gpio.cpp",
        "itoa.c",
        "linux/esp_rom_sys.c",
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
    ]
)
