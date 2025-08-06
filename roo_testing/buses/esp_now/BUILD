
cc_library(
    name = "esp_now",
    srcs = ["fake_esp_now.cpp"],
    hdrs = ["fake_esp_now.h"],
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/sys",
    ]
)

cc_library(
    name = "include",
    hdrs = ["fake_esp_now.h"],
    visibility = ["//visibility:public"],
    linkstatic = 1,
    deps = [
        "//roo_testing/sys:include",
    ]
)

