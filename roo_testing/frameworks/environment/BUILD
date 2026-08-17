load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//roo_testing/platforms:profile_guard.bzl", "profile_guard")

# These targets validate and constrain the globally selected build profile.
# They intentionally do not export defines: build identity must be identical in
# every target-configuration compile action, including dependency sublibraries.
profile_guard(
    name = "emulator_platform_guard",
    environment = "The roo_testing emulator",
    required_constraints = ["//roo_testing/platforms:roo_testing"],
)

cc_library(
    name = "emulator",
    srcs = ["emulator_profile_contract.c"],
    copts = ["-Werror"],
    data = [":emulator_platform_guard"],
    visibility = ["//visibility:public"],
)

profile_guard(
    name = "esp_idf_platform_guard",
    environment = "The roo_testing ESP-IDF environment",
    required_constraints = [
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "esp_idf",
    srcs = ["esp_idf_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":esp_idf_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [
        ":emulator",
        "//roo_testing/soc:target",
    ],
)

# IDF-only entry points depend on this target so they cannot silently build
# under the Arduino frontend, even though both frontends expose ESP-IDF APIs.
profile_guard(
    name = "idf_platform_guard",
    environment = "The roo_testing IDF-only ESP32 environment",
    required_constraints = [
        "//roo_testing/platforms:esp32",
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:idf",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "idf",
    data = [":idf_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":esp_idf"],
)

profile_guard(
    name = "arduino_platform_guard",
    environment = "The roo_testing Arduino ESP32 environment",
    required_constraints = [
        "//roo_testing/platforms:arduino",
        "//roo_testing/platforms:esp32",
        "//roo_testing/platforms:esp_idf",
        "//roo_testing/platforms:roo_testing",
    ],
)

cc_library(
    name = "arduino",
    srcs = ["arduino_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":arduino_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":esp_idf"],
)
