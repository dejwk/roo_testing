
cc_library(
    name = "fltk_viewport",
    srcs = [
        "fltk_viewport.cpp",
    ],
    hdrs = [
        "fltk_viewport.h",
    ],
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
