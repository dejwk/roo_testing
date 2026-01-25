load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "timer",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "include",
    hdrs = glob(["*.h"]),
    defines = ["CORE_DEBUG_LEVEL=5"],
    visibility = ["//visibility:public"],
)
