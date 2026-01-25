load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "include",
    hdrs = glob(["**/*.h"]),
    includes = [
        ".",
        "esp32",
    ],
    visibility = ["//visibility:public"],
)
