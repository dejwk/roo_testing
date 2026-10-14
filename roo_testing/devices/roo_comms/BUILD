load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

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
    visibility = ["//visibility:public"],
    deps = [
        "//roo_testing/buses/esp_now",
        # "//roo_testing/transducers/voltage",
        "@glog",
    ],
)

cc_library(
    name = "environmental_sensor",
    srcs = ["environmental_sensor.cpp"],
    hdrs = ["environmental_sensor.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":roo_comms_cc_proto",
        "//roo_testing/buses/esp_now",
        "//roo_testing/sys",
        "//roo_testing/transducers/temperature",
        "@glog",
    ],
)

cc_test(
    name = "wire_compatibility_test",
    size = "small",
    srcs = ["wire_compatibility_test.cpp"],
    deps = [
        ":roo_comms_cc_proto",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "environmental_sensor_test",
    size = "small",
    srcs = ["environmental_sensor_test.cpp"],
    deps = [
        ":environmental_sensor",
        "//:arduino_gtest_main",
        "//roo_testing/microcontrollers/esp32",
    ],
)
