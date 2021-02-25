

cc_library(
    name = "time",
    srcs = ["clock.cpp"],
    hdrs = ["clock.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "default_system_clock",
    srcs = ["default_system_clock.cpp"],
    deps = [":time"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "fake_system_clock",
    srcs = ["fake_system_clock.cpp"],
    deps = [":time"],
    visibility = ["//visibility:public"],
)
