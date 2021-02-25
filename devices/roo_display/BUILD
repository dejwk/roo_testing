cc_library(
    name = "roo_display",
    hdrs = [
        "fltk_device.h",
    ],
    deps = ["@fltk//:fltk"],
#    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "src",
    srcs = [
        "fltk_device.cpp",
        "fltk_device.h",
    ],
    visibility = ["//visibility:public"],
)
