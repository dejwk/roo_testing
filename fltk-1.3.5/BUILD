load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "fltk",
    srcs = [
        "lib/libfltk.a",
        "lib/libfltk_images.a",
    ],
    hdrs = glob(["include/FL/*.h"]) + glob(["include/FL/*.H"]),
    includes = ["include"],
    linkopts = ["-lX11 -ldl -lm -lpthread"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    alwayslink = 1,
    # deps = ["@system_libs//:x11"],
)
