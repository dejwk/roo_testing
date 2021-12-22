cc_library(
    name = "roo_display",
    visibility = ["//visibility:public"],
    deps = [":fltk_framebuffer"],
)

cc_library(
    name = "fltk_framebuffer",
    srcs = [
        "fltk_framebuffer.cpp",
    ],
    hdrs = [
        "fltk_framebuffer.h",
    ],
    #    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = ["@fltk"],
)

filegroup(
    name = "src",
    srcs = [
        "fltk_device.cpp",
        "fltk_device.h",
    ],
    visibility = ["//visibility:public"],
)
