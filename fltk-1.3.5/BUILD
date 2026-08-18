load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "fltk",
    srcs = ["lib/libfltk.a"],
    hdrs = glob(["include/FL/*.h"]) + glob(["include/FL/*.H"]),
    includes = ["include"],
    linkopts = [
        "-lX11",
        "-ldl",
        "-lm",
        "-lpthread",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    alwayslink = 1,
    # deps = ["@system_libs//:x11"],
)

cc_test(
    name = "core_link_test",
    size = "small",
    srcs = ["core_link_test.cpp"],
    deps = [":fltk"],
)
