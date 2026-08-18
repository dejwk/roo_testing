load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "fltk",
    srcs = [
        # Static linker order: image codecs first, then the FLTK core symbols
        # that they reference. This matches `fltk-config --use-images`.
        # do not sort
        "lib/libfltk_images.a",
        "lib/libfltk.a",
    ],
    hdrs = glob(["include/FL/*.h"]) + glob(["include/FL/*.H"]),
    includes = ["include"],
    linkopts = [
        "-lX11",
        "-ldl",
        "-lm",
        "-lpthread",
        "-ljpeg",
        "-lpng",
        "-lz",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    alwayslink = 1,
    # deps = ["@system_libs//:x11"],
)

cc_test(
    name = "image_link_test",
    size = "small",
    srcs = ["image_link_test.cpp"],
    deps = [":fltk"],
)
