load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "littlefs",
    srcs = glob(["*.c"]),
    hdrs = glob(["**/*.h"]),
    includes = [
        ".",
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        # "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spi_flash",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
    ],
)

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        ".",
        "include",
    ],
    visibility = ["//visibility:public"],
)
