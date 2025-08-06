cc_library(
    name = "esp32",
    srcs = glob(["**/*.cpp"]) + glob(["**/*.c"]),
    hdrs = glob(["**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/devices/microcontroller/esp32:core",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/soc",
    ]
)
