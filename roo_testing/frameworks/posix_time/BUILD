load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "posix_time",
    srcs = ["posix_time.cpp"],
    hdrs = ["posix_time.h"],
    copts = ["-fno-exceptions"],
    linkopts = ["-Wl,--wrap=" + name for name in [
        "gettimeofday",
        "settimeofday",
        "time",
        "clock_gettime",
        "clock_getres",
        "clock_settime",
        "adjtime",
    ]],
    visibility = ["//visibility:public"],
    deps = [
        ":host_time",
        "//roo_testing/system:wall_clock",
    ],
    alwayslink = True,
)

[
    cc_test(
        name = name,
        srcs = ["posix_time_test.cpp"],
        deps = [
            ":posix_time",
            "//roo_testing/system:timer",
            "@googletest//:gtest_main",
        ] + mode,
    )
    for name, mode in [
        (
            "posix_time_test",
            ["//roo_testing/system:manual_time_mode"],
        ),
        ("posix_time_auto_test", []),
    ]
]

# Server time can be sampled without enabling device-clock interposition.
cc_library(
    name = "host_time",
    srcs = ["host_time.cpp"],
    hdrs = ["posix_time.h"],
    visibility = ["//visibility:public"],
)
