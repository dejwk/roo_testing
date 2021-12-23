# cc_library(
#     name = "roo_display",
#     visibility = ["//visibility:public"],
#     deps = [":fltk_viewport"],
# )

cc_library(
    name = "fltk_viewport",
    srcs = [
        "fltk_viewport.cpp",
    ],
    hdrs = [
        "fltk_viewport.h",
    ],
    #    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        ":viewport",
        "@fltk",
    ],
)

cc_library(
    name = "viewport",
    hdrs = [
        "viewport.h",
    ],
    #    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "flex_viewport",
    srcs = [
        "flex_viewport.cpp",
    ],
    hdrs = [
        "flex_viewport.h",
    ],
    visibility = ["//visibility:public"],
    deps = [":viewport"],
)

filegroup(
    name = "src",
    srcs = [
        "fltk_device.cpp",
        "fltk_device.h",
    ],
    visibility = ["//visibility:public"],
)
