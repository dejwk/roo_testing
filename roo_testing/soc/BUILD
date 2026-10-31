load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//roo_testing/platforms:profile_guard.bzl", "profile_guard")

# Keep each supported SoC in its own concrete target. Build-wide identity is
# owned by the matching platform/bazelrc profile; this target supplies the
# typed header and rejects a contradictory target platform.
profile_guard(
    name = "esp32_platform_guard",
    environment = "The roo_testing classic ESP32 SoC profile",
    required_constraints = ["//roo_testing/platforms:esp32"],
)

cc_library(
    name = "esp32",
    srcs = ["esp32_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":esp32_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":identity"],
)

# Framework and emulator targets share this platform-selected profile.
alias(
    name = "target",
    actual = select({
        "//roo_testing/platforms:is_esp32s3": ":esp32s3",
        "//roo_testing/platforms:is_esp32c3": ":esp32c3",
        "//conditions:default": ":esp32",
    }),
    visibility = ["//visibility:public"],
)

profile_guard(
    name = "esp32s3_platform_guard",
    environment = "The roo_testing ESP32S3 SoC profile",
    required_constraints = ["//roo_testing/platforms:esp32s3"],
)

cc_library(
    name = "esp32s3",
    srcs = ["esp32s3_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":esp32s3_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":identity"],
)

profile_guard(
    name = "esp32c3_platform_guard",
    environment = "The roo_testing ESP32C3 SoC profile",
    required_constraints = ["//roo_testing/platforms:esp32c3"],
)

cc_library(
    name = "esp32c3",
    srcs = ["esp32c3_profile_contract.cc"],
    copts = ["-Werror"],
    data = [":esp32c3_platform_guard"],
    visibility = ["//visibility:public"],
    deps = [":identity"],
)

# Generic identity has no frontend or ESP-IDF capability requirement. Concrete
# :target contracts above add ESP-specific checks without changing this API.
cc_library(
    name = "identity",
    srcs = ["soc_profile_contract.cc"],
    hdrs = [
        "include/roo_testing/esp_idf_soc_profile.h",
        "include/roo_testing/soc_profile.h",
    ],
    copts = ["-Werror"],
    data = select({
        "//roo_testing/platforms:is_esp32s3": [":esp32s3_platform_guard"],
        "//roo_testing/platforms:is_esp32c3": [":esp32c3_platform_guard"],
        "//conditions:default": [":esp32_platform_guard"],
    }),
    local_defines = select({
        "//roo_testing/platforms:is_esp32s3": ["ROO_TESTING_EXPECT_SOC_ESP32S3=1"],
        "//roo_testing/platforms:is_esp32c3": ["ROO_TESTING_EXPECT_SOC_ESP32C3=1"],
        "//conditions:default": ["ROO_TESTING_EXPECT_SOC_ESP32=1"],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
