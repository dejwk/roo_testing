load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:private"])

# Neither library has a framework or roo_testing dependency. Their source files
# prove that the selected compiler profile reaches unrelated C and C++ actions.
cc_library(
    name = "unrelated_c",
    srcs = [
        "esp32_idf_environment_contract.h",
        "global_environment_contract.h",
        "unrelated_c.c",
    ],
    copts = ["-Werror"],
)

cc_library(
    name = "unrelated_cpp",
    srcs = [
        "esp32_idf_environment_contract.h",
        "global_environment_contract.h",
        "unrelated_cpp.cc",
    ],
    copts = ["-Werror"],
)

cc_test(
    name = "global_environment_test",
    size = "small",
    srcs = ["global_environment_test.cc"],
    copts = ["-Werror"],
    deps = [
        ":unrelated_c",
        ":unrelated_cpp",
    ],
)

cc_library(
    name = "arduino_optional_dependency",
    srcs = ["arduino_optional_dependency.cc"],
)

# Successful linking proves that the public is_arduino config_setting selected
# the optional dependency for the current target platform.
cc_library(
    name = "select_probe",
    srcs = ["select_probe.cc"],
    deps = select({
        "//roo_testing/platforms:is_arduino": [":arduino_optional_dependency"],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "arduino_select_test",
    size = "small",
    srcs = ["arduino_select_test.cc"],
    deps = [":select_probe"],
)

# IDF-only integration targets are run by verify_profile.sh under the explicit
# roo_testing_idf_esp32 configuration. They are excluded from the Arduino
# profile's ordinary //:all_tests suite.
cc_library(
    name = "idf_unrelated_c",
    srcs = [
        "esp32_idf_environment_contract.h",
        "idf_environment_contract.h",
        "idf_unrelated_c.c",
    ],
    copts = ["-Werror"],
    tags = ["manual"],
)

cc_library(
    name = "idf_unrelated_cpp",
    srcs = [
        "esp32_idf_environment_contract.h",
        "idf_environment_contract.h",
        "idf_unrelated_cpp.cc",
    ],
    copts = ["-Werror"],
    tags = ["manual"],
)

cc_test(
    name = "idf_global_environment_test",
    size = "small",
    srcs = ["idf_global_environment_test.cc"],
    copts = ["-Werror"],
    tags = ["manual"],
    deps = [
        ":idf_unrelated_c",
        ":idf_unrelated_cpp",
    ],
)

cc_test(
    name = "idf_sdkconfig_purity_test",
    size = "small",
    srcs = [
        "esp32_idf_environment_contract.h",
        "idf_environment_contract.h",
        "idf_sdkconfig_purity_test.cc",
    ],
    tags = ["manual"],
    deps = ["//roo_testing/frameworks/esp-idf:sdkconfig"],
)

cc_library(
    name = "idf_capability_dependency",
    srcs = ["idf_capability_dependency.cc"],
    tags = ["manual"],
)

cc_library(
    name = "idf_frontend_dependency",
    srcs = ["idf_frontend_dependency.cc"],
    tags = ["manual"],
)

cc_library(
    name = "unexpected_arduino_selection",
    srcs = ["unexpected_arduino_selection.cc"],
    tags = ["manual"],
)

cc_library(
    name = "idf_select_probe",
    srcs = ["idf_select_probe.cc"],
    tags = ["manual"],
    deps = select({
        "//roo_testing/platforms:is_esp_idf": [":idf_capability_dependency"],
        "//conditions:default": [],
    }) + select({
        "//roo_testing/platforms:is_idf": [":idf_frontend_dependency"],
        "//conditions:default": [],
    }) + select({
        "//roo_testing/platforms:is_arduino": [":unexpected_arduino_selection"],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "idf_select_test",
    size = "small",
    srcs = ["idf_select_test.cc"],
    tags = ["manual"],
    deps = [":idf_select_probe"],
)

cc_test(
    name = "esp_idf_gtest_main_test",
    size = "small",
    srcs = ["esp_idf_gtest_main_test.cc"],
    tags = ["manual"],
    deps = ["//:esp_idf_gtest_main"],
)

cc_test(
    name = "esp_idf_main_test",
    size = "small",
    srcs = ["esp_idf_main_test.cc"],
    tags = ["manual"],
    deps = ["//:esp_idf_main"],
)

# Built explicitly by verify_profile.sh with an additional user --copt.
cc_library(
    name = "user_copt_probe",
    srcs = [
        "esp32_idf_environment_contract.h",
        "global_environment_contract.h",
        "user_copt_probe.cc",
    ],
    tags = ["manual"],
)

# Built explicitly by verify_profile.sh with --config=asan.
cc_library(
    name = "asan_profile_probe",
    srcs = [
        "asan_profile_probe.cc",
        "esp32_idf_environment_contract.h",
        "global_environment_contract.h",
    ],
    tags = ["manual"],
)

# Used by the negative integration check. It deliberately advertises Arduino
# on an unsupported SoC, so the framework's hard platform guard rejects it.
constraint_value(
    name = "unsupported_soc",
    constraint_setting = "//roo_testing/platforms:soc",
)

platform(
    name = "incorrect_arduino_soc",
    constraint_values = [
        ":unsupported_soc",
        "//roo_testing/platforms:arduino",
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:roo_testing",
    ],
    parents = ["@platforms//host:host"],
)

exports_files(["verify_profile.sh"])
