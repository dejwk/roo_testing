cc_library(
    name = "fatfs",
    srcs = glob(["src/*.c"]),
    hdrs =
        glob(["src/*.h"]) +
        glob(["vfs/*.h"]) +
        glob(["diskio/*.h"]),
    includes = [
        ".",
        "src",
        "vfs",
        "diskio",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/wear_levelling",
    ],
)
