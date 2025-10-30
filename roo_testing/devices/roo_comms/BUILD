proto_library(
  name = "roo_comms_proto",
  srcs = ["roo_comms.proto"],
)

cc_proto_library(
  name = "roo_comms_cc_proto",
  deps = [":roo_comms_proto"],
)

cc_library(
    name = "relay",
    srcs = ["relay.cpp"],
    hdrs = ["relay.h"],
    deps = [
        "//roo_testing/buses/esp_now",
        # "//roo_testing/transducers/voltage",
       "@glog//:glog",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "environmental_sensor",
    srcs = ["environmental_sensor.cpp"],
    hdrs = ["environmental_sensor.h"],
    deps = [
        "//roo_testing/buses/esp_now",
        "//roo_testing/sys",
        "//roo_testing/transducers/temperature",
        "@glog//:glog",
        ":roo_comms_cc_proto"
    ],
    visibility = ["//visibility:public"],
)

