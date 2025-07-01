cc_library(
    name = "time",
    srcs = ["clock.cpp"],
    hdrs = ["clock.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/transducers",
        ],
)

cc_library(
    name = "default_system_clock",
    hdrs = ["default_system_clock.h"],
    srcs = ["default_system_clock.cpp"],
    visibility = ["//visibility:public"],
    deps = [":time",
            "//roo_testing/sys"],
)

cc_library(
    name = "fake_system_clock",
    srcs = ["fake_system_clock.cpp"],
    visibility = ["//visibility:public"],
    deps = [":time"],
)
