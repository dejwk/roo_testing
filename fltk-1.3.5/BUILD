cc_library(
    name = "fltk",
    srcs = ["lib/libfltk.a", "lib/libfltk_images.a"],
    hdrs = glob(["include/FL/*.h"]) + glob(["include/FL/*.H"]),
    includes = ["include"],
    alwayslink = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    linkopts = ["-lX11 -ldl -lm -lpthread"],
    # deps = ["@system_libs//:x11"],
)
