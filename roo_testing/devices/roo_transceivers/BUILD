cc_library(
    name = "relay",
    srcs = ["relay.cpp"],
    hdrs = ["relay.h"],
    deps = [
        "//roo_testing/buses/roo_transceivers",
        "//roo_testing/transducers/voltage",
#        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "environmental_sensor",
    srcs = ["environmental_sensor.cpp"],
    hdrs = ["environmental_sensor.h"],
    deps = [
        "//roo_testing/buses/roo_transceivers",
        "//roo_testing/transducers/temperature",
#        "@com_github_google_glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

