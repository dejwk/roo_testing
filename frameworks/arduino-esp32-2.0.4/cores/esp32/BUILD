cc_library(
    name = "esp32",
    srcs = 
        glob(["**/*.c"]) +
        glob(["**/*.cpp"], exclude = ["main.cpp"]) + 
        glob(["tools/sdk/src/*.c"]) +
        glob(["**/*.h"]),
    hdrs = glob(["**/*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    includes = ["."],
    linkstatic = 1,
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/tools/sdk/esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf/include",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
    ]
)

cc_library(
    name = "main",
    srcs = ["main.cpp"],
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
    alwayslink = 1,
    deps = [
        ":esp32",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc/esp32",
        ],
)
