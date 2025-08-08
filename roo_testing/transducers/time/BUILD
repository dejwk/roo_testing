cc_library(
    name = "time",
    srcs = ["clock.cpp", "default_system_clock.cpp"],
    hdrs = ["clock.h", "default_system_clock.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/transducers",
        "//roo_testing/system:timer"
        ],
)

# cc_library(
#     name = "default_system_clock",
#     hdrs = ["default_system_clock.h"],
#     srcs = ["default_system_clock.cpp"],
#     visibility = ["//visibility:public"],
#     deps = [":time",
#             "//roo_testing/system:timer"],
# )

# cc_library(
#     name = "fake_system_clock",
#     srcs = ["fake_system_clock.cpp"],
#     visibility = ["//visibility:public"],
#     deps = [":time"],
# )
