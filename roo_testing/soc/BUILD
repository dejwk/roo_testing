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
    hdrs = ["include/roo_testing/soc_profile.h"],
    copts = ["-Werror"],
    data = [":esp32_platform_guard"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
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
    hdrs = ["include/roo_testing/soc_profile.h"],
    copts = ["-Werror"],
    data = [":esp32s3_platform_guard"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

profile_guard(
    name = "esp32c3_platform_guard",
    environment = "The roo_testing ESP32C3 SoC profile",
    required_constraints = ["//roo_testing/platforms:esp32c3"],
)

cc_library(
    name = "esp32c3",
    srcs = ["esp32c3_profile_contract.cc"],
    hdrs = ["include/roo_testing/soc_profile.h"],
    copts = ["-Werror"],
    data = [":esp32c3_platform_guard"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
