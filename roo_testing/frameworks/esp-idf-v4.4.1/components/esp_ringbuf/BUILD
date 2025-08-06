cc_library(
    name = "esp_ringbuf",
    srcs = glob(["*.c"]),
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
    ],
)
