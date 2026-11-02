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
./test/profile/verify_identity.sh

run_bazel() {
  "${bazel_bin}" --output_user_root="${test_root}/configured" "$@"
}

# IDF integration deliberately suppresses the workspace rc and loads only the
# shared base plus the IDF frontend. The wrapper sees the explicit IDF config
# and does not add its Arduino default. The user's home rc remains active.
run_idf_bazel() {
  "${bazel_bin}" \
    --output_user_root="${test_root}/configured" \
    --noworkspace_rc \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32/base.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32/idf.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32s3/base.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32s3/idf.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32c3/base.bazelrc" \
    --bazelrc="${workspace}/.roo_testing/bazelrc/esp32c3/idf.bazelrc" \
    "$@" \
    --repository_cache="${repository_cache}" \
    --registry=https://raw.githubusercontent.com/dejwk/roo-registry/main/ \
    --registry=https://bcr.bazel.build
}

run_without_profile() {
  "${bazel_bin}" \
    --output_user_root="${test_root}/configured" \
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

# Verifies root/recursive wildcards skip generated output directories. The
# deliberately broken package must never be loaded, even after local builds.
verify_wildcards() {
  local fixture
  fixture="$(mktemp -d "${test_root}/wildcard-output.XXXXXX")"
  local generated_link="${workspace}/test/profile/bazel-${fixture##*/}"
  printf '%s\n' 'fail("Wildcard entered generated Bazel output")' >"${fixture}/BUILD"
  ln -s "${fixture}" "${generated_link}"
  local status=0
  run_bazel test //:all //... --test_output=errors "$@" || status=$?
  rm -- "${generated_link}"
  rm -r -- "${fixture}"
  return "${status}"
}

verify_wildcards

run_idf_bazel test //:idf_tests --config=roo_testing_idf_esp32
verify_wildcards --config=roo_testing_idf_esp32

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
  ROO_TESTING_RUNTIME_FREERTOS
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
  grep -Fxq \
    'import %workspace%/.roo_testing/bazelrc/common/asan.bazelrc' \
    "examples/${example}/.bazelrc"
  cmp .roo_testing/bin/bazel \
    "examples/${example}/.roo_testing/bin/bazel"
  test -x "examples/${example}/.roo_testing/bin/bazel"
  cmp .roo_testing/bazelrc/common/asan.bazelrc \
    "examples/${example}/.roo_testing/bazelrc/common/asan.bazelrc"
  cmp .roo_testing/bazelrc/esp32/base.bazelrc \
    "examples/${example}/.roo_testing/bazelrc/esp32/base.bazelrc"
  cmp .roo_testing/bazelrc/esp32/arduino.bazelrc \
    "examples/${example}/.roo_testing/bazelrc/esp32/arduino.bazelrc"
done

# Additional SoCs must satisfy the same contracts in both frontends.
for soc in esp32s3 esp32c3; do
  run_bazel test //:all_tests "--config=roo_testing_arduino_${soc}"
  run_idf_bazel test //:idf_tests "--config=roo_testing_idf_${soc}"
  run_bazel build //test/profile:user_copt_probe "--config=roo_testing_arduino_${soc}" \
    --copt=-DROO_TESTING_USER_COPT_PROBE=73
  for frontend in arduino idf; do
    run_bazel build //test/profile:asan_profile_probe "--config=roo_testing_${frontend}_${soc}" --config=asan
    arch=XTENSA
    [[ "${soc}" != esp32c3 ]] || arch=RISCV
    soc_upper="${soc^^}"
    definitions=(ROO_TESTING_RUNTIME_FREERTOS ROO_TESTING ESP_PLATFORM IDF_VER CONFIG_IDF_FIRMWARE_CHIP_ID CONFIG_IDF_TARGET CONFIG_IDF_TARGET_ARCH "CONFIG_IDF_TARGET_ARCH_${arch}" "CONFIG_IDF_TARGET_${soc_upper}" ESP32 ROO_TESTING_SOC "ROO_TESTING_SOC_${soc_upper}")
    for language in c cpp; do
      target="unrelated_${language}"
      [[ "${frontend}" != idf ]] || target="idf_${target}"
      aquery_log="${test_root}/${soc}-${frontend}-${language}.aquery"
      run_bazel aquery "mnemonic('(Cpp|C)Compile', //test/profile:${target})" \
        "--config=roo_testing_${frontend}_${soc}" --include_commandline >"${aquery_log}"
      for definition in "${definitions[@]}"; do
        count="$( { grep -F -o -- "-D${definition}=" "${aquery_log}" || true; } | wc -l)"
        [[ "${count}" -eq 1 ]] || { echo "Incorrect ${definition} count for ${soc}/${frontend}/${language}" >&2; exit 1; }
      done
      if [[ "${frontend}" == arduino ]]; then
        grep -Fq -- "-DARDUINO_${soc_upper}_DEV=1" "${aquery_log}"
      elif grep -Eq -- '-D(ARDUINO[^=]*|CORE_DEBUG_LEVEL)=' "${aquery_log}"; then
        echo "Arduino identity leaked into IDF" >&2; exit 1
      fi
    done
    expect_failure "mismatched SoC platform and macros" \
      run_bazel build //roo_testing/soc:target "--config=roo_testing_${frontend}_${soc}" \
      --platforms=//roo_testing/platforms:idf_esp32
  done
  for example in gpio onewire rtc_ds3231_i2c simple tft_display tft_touch; do
    for fragment in base arduino idf; do
      cmp ".roo_testing/bazelrc/${soc}/${fragment}.bazelrc" \
        "examples/${example}/.roo_testing/bazelrc/${soc}/${fragment}.bazelrc"
    done
  done
done

echo "roo_testing six ESP32 global compiler profiles verified"

# Check the same host console bridge through each ESP32 board's real pin route.
for soc in esp32 esp32s3 esp32c3; do
  for frontend in arduino idf; do
    console_profile="--config=roo_testing_${frontend}_${soc}"
    run_bazel build //test:esp32_console_probe "${console_profile}"
    console_binary="$(run_bazel cquery //test:esp32_console_probe "${console_profile}" --output=files)"
    python3 test/uart_console_test.py "${console_binary}" --nonblocking-eof
  done
done

python3 test/pico/verify_pico.py

python3 test/arduino_pico/verify_arduino_pico.py
echo "roo_testing eight global compiler profiles verified"
