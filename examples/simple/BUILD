load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = glob(["src/**/*.cpp"]),
    deps = [
        "@roo_testing//roo_testing/frameworks/arduino-esp32-2.0.4/cores/esp32:main",
    ],
)
