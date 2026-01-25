cc_library(
    name = "spiffs",
    srcs = glob(["*.c"]) + [
        "spiffs/src/spiffs_cache.c",
        "spiffs/src/spiffs_hydrogen.c",
    ],
    hdrs = glob(["*.h"]) + glob(["include/**/*.h"]) + [
        "spiffs/src/spiffs.h",
        "spiffs/src/spiffs_nucleus.h",
    ],
    includes = [
        ".",
        "include",
        "spiffs/src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spi_flash",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["*.h"]) + glob(["include/**/*.h"]) + [
        "spiffs/src/spiffs.h",
        "spiffs/src/spiffs_nucleus.h",
    ],
    includes = [
        ".",
        "include",
        "spiffs/src",
    ],
    visibility = ["//visibility:public"],
)
