cc_library(
    name = "vfs",
    srcs = [
        "vfs.c",
    ],
    hdrs = glob(["include/**/*.h"]) + glob(["private_include/**/*.h"]),
    includes = [
        ".",
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:uart",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_ringbuf",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_rom",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_timer",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:dirent",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:errno",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:reent",
    ],
)
