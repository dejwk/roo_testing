cc_library(
    name = "pthread",
    srcs = glob(["*.c"]),
    #    srcs = glob(["*.h"]) + [
    #        "spiffs/src/spiffs_hydrogen.c",
    #        "spiffs/src/spiffs_cache.c",
    #    ],
    hdrs = glob(["include/**/*.h"]),
    includes = [
        ".",
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/esp_common",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos"
    ],
)
