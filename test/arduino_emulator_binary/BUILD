load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//roo_testing/emulation:arduino.bzl", "roo_arduino_example")

cc_library(
    name = "sketch_support",
    hdrs = ["fixture/sketch_support.h"],
    includes = ["fixture"],
)

roo_arduino_example(
    name = "runnable_sketch",
    copts = ["-DROO_TESTING_SKETCH_COPT=29"],
    defines = ["ROO_TESTING_SKETCH_DEFINE=71"],
    sketch = "fixture/runnable_sketch.ino",
    tags = ["manual"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [":sketch_support"],
)
