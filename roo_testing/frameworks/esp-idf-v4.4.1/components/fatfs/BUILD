load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "fatfs",
    srcs =
        glob(
            ["src/*.c"],
            exclude = [
                "src/diskio.c",
                "src/ffsystem.c",
            ],
        ) + [
            "diskio/diskio.c",
            "diskio/diskio_sdmmc.c",
            "port/freertos/ffsystem.c",
            "vfs/vfs_fat.c",
            "vfs/vfs_fat_sdmmc.c",
        ],
    hdrs =
        glob(["src/*.h"]) + glob([
            "vfs/*.h",
        ]) + glob([
            "diskio/*.h",
        ]),
    includes = [
        ".",
        "diskio",
        "src",
        "vfs",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1:sdkconfig",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:sdmmc",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/driver:sdspi",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/freertos",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/vfs",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/wear_levelling",
    ],
)

cc_library(
    name = "include",
    hdrs =
        glob(["src/*.h"]) + glob([
            "vfs/*.h",
        ]) + glob([
            "diskio/*.h",
        ]),
    includes = [
        ".",
        "diskio",
        "src",
        "vfs",
    ],
    visibility = ["//visibility:public"],
)
