load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "wear_levelling",
    srcs = glob(["*.cpp"]),
    hdrs =
        glob(["*.h"]) + glob([
            "include/**/*.h",
        ]) + glob([
            "private_include/**/*.h",
        ]),
    includes = [
        ".",
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/log",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/newlib:locks",
        "//roo_testing/frameworks/esp-idf-v4.4.1/components/spi_flash",
    ],
)

cc_library(
    name = "include",
    hdrs =
        glob(["*.h"]) + glob([
            "include/**/*.h",
        ]) + glob([
            "private_include/**/*.h",
        ]),
    includes = [
        ".",
        "include",
        "private_include",
    ],
    visibility = ["//visibility:public"],
)
