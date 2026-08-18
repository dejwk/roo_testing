cc_library(
    name = "fltk",
    srcs = ["lib/libfltk.a"],
    hdrs = glob(["FL/*.h"]) + glob(["FL/*.H"]),
    includes = ["."],
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = ["@system_libs//:x11"],
    alwayslink = 1,
)
