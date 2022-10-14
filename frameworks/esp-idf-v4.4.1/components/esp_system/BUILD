cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_chip_info",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_random",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_hw_support:esp_mac",
    ]
)
