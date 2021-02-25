cc_library(
    name = "testing",
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino_main",
    srcs = glob(["arduino_main.cpp"]),
    visibility = ["//visibility:public"],
    deps = [
        ":arduino",
    ],
)

cc_library(
    name = "arduino",
    visibility = ["//visibility:public"],
    deps = [
        "//testing/frameworks/arduinoespressif32:arduino",
    ],
)
