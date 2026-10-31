load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:private"])

# Wrong profiles skip these tests in wildcard builds; public :pico_sdk itself
# has a hard guard and fails when explicitly requested with no SDK capability.
cc_library(
    name = "unrelated_c",
    srcs = [
        "profile_contract.h",
        "unrelated_c.c",
    ],
    copts = ["-Werror"],
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
)

cc_test(
    name = "sdk_probe_c",
    size = "small",
    srcs = [
        "profile_contract.h",
        "sdk_probe.c",
    ],
    copts = ["-Werror"],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = ["//:pico_sdk"],
)

cc_test(
    name = "sdk_probe_cpp",
    size = "small",
    srcs = [
        "profile_contract.h",
        "sdk_probe.cc",
    ],
    copts = ["-Werror"],
    linkstatic = True,
    target_compatible_with = ["//roo_testing/platforms:pico_sdk"],
    deps = [
        ":unrelated_c",
        "//:pico_sdk",
    ],
)

cc_binary(
    name = "unsupported_sleep_probe",
    srcs = ["unsupported_sleep_probe.c"],
    tags = ["manual"],
    deps = ["//:pico_sdk"],
)

exports_files(["verify_pico.py"])
