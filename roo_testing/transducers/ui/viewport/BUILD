load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

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

cc_test(
    name = "flex_viewport_test",
    size = "small",
    srcs = ["flex_viewport_test.cpp"],
    deps = [":flex", "@googletest//:gtest_main"],
)
