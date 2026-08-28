load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "synchronization",
    srcs = ["scheduler_safe_host_lock.cpp"],
    hdrs = ["scheduler_safe_host_lock.h"],
    visibility = [
        "//roo_testing:__subpackages__",
        "//test:__pkg__",
    ],
    deps = ["@glog"],
)
