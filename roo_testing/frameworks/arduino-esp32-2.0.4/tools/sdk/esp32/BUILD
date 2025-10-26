cc_library(
    name = "esp32",
    # srcs = glob(["*hal*.c"]) + glob(
    #     ["**/*.cpp"],
    #     exclude = ["main.cpp"],
    # ) + glob([
    #     "tools/sdk/src/*.c",
    # ]),
    srcs = glob(["**/*.cpp"]),
    hdrs = glob(["**/*.h"]),
    defines = [
        "CORE_DEBUG_LEVEL=5",
        "CONFIG_IDF_TARGET_LINUX",
    ],
    includes = [
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/arduino-esp32-2.0.4/variants:esp32",
        "@glog",
        "//roo_testing/frameworks/esp-idf-v4.4.1"
    ],
)
