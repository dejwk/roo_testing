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
        "//roo_testing/transducers/ui/viewport",
    ],
)
