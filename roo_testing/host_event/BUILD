load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "host_event",
    srcs = ["host_event_endpoint.cpp"],
    hdrs = ["host_event_endpoint.h"],
    strip_include_prefix = ".",
    include_prefix = "roo_testing/host_event",
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/frameworks/esp-idf/components/freertos:include",
    ],
)
