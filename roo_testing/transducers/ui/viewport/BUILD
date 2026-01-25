load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "viewport",
    hdrs = [
        "viewport.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "flex",
    srcs = [
        "flex_viewport.cpp",
    ],
    hdrs = [
        "flex_viewport.h",
    ],
    visibility = ["//visibility:public"],
    deps = [":viewport"],
)
