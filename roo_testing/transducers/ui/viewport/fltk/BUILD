load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "fltk",
    srcs = [
        "fltk_viewport.cpp",
    ],
    hdrs = [
        "fltk_viewport.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//fltk-1.3.5:fltk",
        "@glog",
        "//roo_testing/transducers/ui/viewport",
    ],
)

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "fltk_viewport_test",
    size = "small",
    srcs = ["fltk_viewport_test.cpp"],
    deps = [":fltk", "@googletest//:gtest_main"],
)
