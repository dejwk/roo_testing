#!/usr/bin/env bash
set -euo pipefail

workspace="${BUILD_WORKSPACE_DIRECTORY:-${PWD}}"
bazel_bin="${BAZEL:-bazel}"
test_root="${ROO_TESTING_PROFILE_TEST_ROOT:-${HOME}/.cache/roo_testing/profile-integration}"
repository_cache="${ROO_TESTING_PROFILE_REPOSITORY_CACHE:-${test_root}/repository-cache}"

for bazel_path in "${test_root}" "${repository_cache}"; do
  resolved_path="$(realpath -m "${bazel_path}")"
  if [[ "${resolved_path}" == /tmp || "${resolved_path}" == /tmp/* ]]; then
    echo "Refusing RAM-backed Bazel path: ${bazel_path}" >&2
    exit 1
  fi
done

mkdir -p "${test_root}" "${repository_cache}"
cd "${workspace}"

./test/profile/verify_bazel_tools.sh

run_bazel() {
  "${bazel_bin}" --output_user_root="${test_root}/configured" "$@"
}

# IDF integration deliberately suppresses the workspace rc and loads only the
# shared base plus the IDF frontend. The wrapper sees the explicit IDF config
# and does not add its Arduino default. The user's home rc remains active.
run_idf_bazel() {
  "${bazel_bin}" \
    --output_user_root="${test_root}/idf-configured" \
    --noworkspace_rc \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32/base.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32/idf.bazelrc" \
    "$@" \
    --repository_cache="${repository_cache}" \
    --registry=https://raw.githubusercontent.com/dejwk/roo-registry/main/ \
    --registry=https://bcr.bazel.build
}

run_without_profile() {
  "${bazel_bin}" \
    --output_user_root="${test_root}/without-profile" \
    --noworkspace_rc \
    "$@" \
    --repository_cache="${repository_cache}" \
    --registry=https://raw.githubusercontent.com/dejwk/roo-registry/main/ \
    --registry=https://bcr.bazel.build
}

expect_failure() {
  local description="$1"
  shift
  if "$@"; then
    echo "Expected failure: ${description}" >&2
    exit 1
  fi
}

run_bazel test \
  //test/profile:global_environment_test \
  //test/profile:arduino_select_test

run_idf_bazel test //:idf_tests --config=roo_testing_idf_esp32

run_bazel build //test/profile:user_copt_probe \
  --copt=-DROO_TESTING_USER_COPT_PROBE=73
run_bazel build //test/profile:asan_profile_probe \
  --config=asan
run_bazel run //test/arduino_emulator_binary:runnable_sketch

macro_target_kind="$(
  run_bazel query //test/arduino_emulator_binary:runnable_sketch \
    --output=label_kind
)"
if [[ "${macro_target_kind}" != \
      "cc_binary rule //test/arduino_emulator_binary:runnable_sketch" ]]; then
  echo "roo_arduino_example did not create a native cc_binary" >&2
  exit 1
fi

expect_failure "Arduino framework without a selected frontend profile" \
  run_without_profile build //:arduino
expect_failure "Arduino framework on the plain host platform" \
  run_bazel build //:arduino \
  --platforms=@platforms//host:host
expect_failure "Arduino framework on an unsupported SoC platform" \
  run_bazel build //:arduino \
  --platforms=//test/profile:incorrect_arduino_soc
expect_failure "Arduino platform with a contradictory compiler macro" \
  run_bazel build //roo_testing/frameworks/environment:arduino \
  --copt=-DARDUINO=99999
expect_failure "Arduino framework under the IDF-only frontend" \
  run_idf_bazel build //:arduino --config=roo_testing_idf_esp32
expect_failure "IDF-only test entry point under the Arduino frontend" \
  run_bazel build //:esp_idf_gtest_main
expect_failure "wrapper rejects simultaneously selected frontends" \
  run_bazel build //test/profile:idf_unrelated_cpp \
  --config=roo_testing_arduino_esp32 \
  --config=roo_testing_idf_esp32
expect_failure "Arduino example binary under the IDF-only frontend" \
  run_idf_bazel build //test/arduino_emulator_binary:runnable_sketch \
  --config=roo_testing_idf_esp32 \
  --noskip_incompatible_explicit_targets

# Ignoring rc files retains the target platform but omits its compiler profile.
# The environment guard must reject that partially configured build.
expect_failure "roo_testing platform without its compiler profile" \
  run_without_profile build //roo_testing/frameworks/environment:arduino \
  --platforms=//roo_testing/platforms:arduino_esp32

base_definitions=(
  ROO_TESTING
  ESP_PLATFORM
  IDF_VER
  CONFIG_IDF_FIRMWARE_CHIP_ID
  CONFIG_IDF_TARGET
  CONFIG_IDF_TARGET_ARCH
  CONFIG_IDF_TARGET_ARCH_XTENSA
  CONFIG_IDF_TARGET_ESP32
  ESP32
  ROO_TESTING_SOC
  ROO_TESTING_SOC_ESP32
)

arduino_definitions=(
  ARDUINO
  ARDUINO_ARCH_ESP32
  ARDUINO_BOARD
  ARDUINO_ESP32_DEV
  ARDUINO_USB_CDC_ON_BOOT
  ARDUINO_USB_DFU_ON_BOOT
  ARDUINO_USB_MSC_ON_BOOT
  ARDUINO_USB_MODE
  ARDUINO_USB_ON_BOOT
  ARDUINO_VARIANT
  CORE_DEBUG_LEVEL
  CONFIG_AUTOSTART_ARDUINO
  CONFIG_ARDUINO_LOOP_STACK_SIZE
  CONFIG_ARDUINO_RUNNING_CORE
  CONFIG_ARDUINO_EVENT_RUNNING_CORE
  CONFIG_ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE
  CONFIG_ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE
  CONFIG_ARDUINO_SERIAL_EVENT_TASK_PRIORITY
  CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
  CONFIG_ARDUHAL_ESP_LOG
)

for target in unrelated_c unrelated_cpp; do
  aquery_log="${test_root}/${target}.aquery"
  run_bazel aquery \
    "mnemonic('(Cpp|C)Compile', //test/profile:${target})" \
    --include_commandline >"${aquery_log}"
  for definition in "${base_definitions[@]}" "${arduino_definitions[@]}"; do
    count="$( { grep -F -o -- "-D${definition}=" "${aquery_log}" || true; } | wc -l)"
    if [[ "${count}" -ne 1 ]]; then
      echo "Expected one -D${definition}= in ${target}; found ${count}" >&2
      exit 1
    fi
  done
done

for target in idf_unrelated_c idf_unrelated_cpp; do
  aquery_log="${test_root}/${target}.aquery"
  run_idf_bazel aquery \
    "mnemonic('(Cpp|C)Compile', //test/profile:${target})" \
    --config=roo_testing_idf_esp32 \
    --include_commandline >"${aquery_log}"
  for definition in "${base_definitions[@]}"; do
    count="$( { grep -F -o -- "-D${definition}=" "${aquery_log}" || true; } | wc -l)"
    if [[ "${count}" -ne 1 ]]; then
      echo "Expected one -D${definition}= in ${target}; found ${count}" >&2
      exit 1
    fi
  done
  for definition in "${arduino_definitions[@]}"; do
    count="$( { grep -F -o -- "-D${definition}=" "${aquery_log}" || true; } | wc -l)"
    if [[ "${count}" -ne 0 ]]; then
      echo "Expected no -D${definition}= in ${target}; found ${count}" >&2
      exit 1
    fi
  done
done

for example in gpio onewire rtc_ds3231_i2c simple tft_display tft_touch; do
  cmp .bazeliskrc "examples/${example}/.bazeliskrc"
  cmp .roo_testing/bin/bazel \
    "examples/${example}/.roo_testing/bin/bazel"
  test -x "examples/${example}/.roo_testing/bin/bazel"
  cmp .roo_testing/bazelrc/esp32/base.bazelrc \
    "examples/${example}/.roo_testing/bazelrc/esp32/base.bazelrc"
  cmp .roo_testing/bazelrc/esp32/arduino.bazelrc \
    "examples/${example}/.roo_testing/bazelrc/esp32/arduino.bazelrc"
done

echo "roo_testing global compiler profile verified"
