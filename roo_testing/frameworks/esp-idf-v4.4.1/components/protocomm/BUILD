load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "include",
    hdrs = glob(["include/**/*.h"]),
    includes = [
        "include/common",
        "include/security",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)
