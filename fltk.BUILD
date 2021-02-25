cc_library(
    name = "fltk",
    srcs = ["lib/libfltk.a", "lib/libfltk_images.a", "lib/libfltk_png.a", "lib/libfltk_z.a", "lib/libfltk_jpeg.a"],
    hdrs = glob(["FL/*.h"]) + glob(["FL/*.H"]),
    includes = ["."],
    alwayslink = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = ["@system_libs//:x11"],
)
