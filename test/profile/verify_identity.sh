#!/usr/bin/env bash
set -euo pipefail

# Focused configuration tests use the same persistent output root as the full
# profile verifier, preserve the user's rc/cache/limits, and serialize Bazel.
workspace="${BUILD_WORKSPACE_DIRECTORY:-${PWD}}"
bazel_bin="${BAZEL:-bazel}"
test_root="${ROO_TESTING_PROFILE_TEST_ROOT:-${HOME}/.cache/roo_testing/profile-integration}"
resolved_root="$(realpath -m "${test_root}")"
if [[ "${resolved_root}" == /tmp || "${resolved_root}" == /tmp/* ]]; then
  echo "Refusing RAM-backed Bazel path: ${test_root}" >&2
  exit 1
fi
mkdir -p "${test_root}"
cd "${workspace}"

run_bazel() {
  "${bazel_bin}" --output_user_root="${test_root}/configured" "$@"
}

# This is only an identity fixture: no new supported backend/profile is added.
generic_options=(
  --platforms=//test/profile:generic_arduino_identity
  --copt=-DROO_TESTING=1
  --copt=-DARDUINO=10819
  '--copt=-DROO_TESTING_SOC="esp32"'
  --copt=-DROO_TESTING_SOC_ESP32=1
  --copt=-DROO_TESTING_RUNTIME_BARE_METAL=1
  --registry=https://raw.githubusercontent.com/dejwk/roo-registry/main/
  --registry=https://bcr.bazel.build
)

run_generic() {
  local command="$1"
  shift
  run_bazel --noworkspace_rc "${command}" "${generic_options[@]}" "$@"
}

expect_failure() {
  local description="$1"
  local diagnostic="$2"
  shift 2
  local log="${test_root}/identity-failure.log"
  if "$@" >"${log}" 2>&1; then
    echo "Expected failure: ${description}" >&2
    exit 1
  fi
  if ! grep -Fq -- "${diagnostic}" "${log}"; then
    cat "${log}" >&2
    echo "Missing expected diagnostic for ${description}: ${diagnostic}" >&2
    exit 1
  fi
}

run_generic test //test/profile:generic_identity_test \
  //test/profile:runtime_selection_test --test_output=errors
run_generic cquery 'deps(//test/profile:generic_identity_test)' \
  >"${test_root}/generic-identity.deps"
if grep -Eq '//roo_testing/frameworks/(esp-idf|arduino-esp32|esp32_shims)' \
    "${test_root}/generic-identity.deps"; then
  echo "Generic identity acquired a framework implementation dependency" >&2
  exit 1
fi

expect_failure "generic identity does not grant IDF capability" \
  'incompatible' \
  run_generic build //roo_testing/frameworks/environment:esp_idf
expect_failure "generic Arduino does not grant Arduino-ESP32 capability" \
  'incompatible' \
  run_generic build //roo_testing/frameworks/environment:arduino_esp32
expect_failure "generic SoC does not grant IDF SoC identity" \
  'Compiler identity does not match selected SoC target' \
  run_generic build //roo_testing/soc:target
expect_failure "generic Arduino still validates its compatibility value" \
  'ARDUINO has an unsupported compatibility value' \
  run_generic build //roo_testing/frameworks/environment:arduino \
  --copt=-UARDUINO --copt=-DARDUINO=99999
expect_failure "runtime macros cannot contradict each other" \
  'Select exactly one roo_testing application runtime' \
  run_generic build //roo_testing/frameworks/environment:runtime \
  --copt=-DROO_TESTING_RUNTIME_FREERTOS=1
expect_failure "bare-metal platform rejects a FreeRTOS compiler identity" \
  'Compiler runtime identity does not match the bare-metal platform' \
  run_generic build //roo_testing/frameworks/environment:runtime \
  --copt=-UROO_TESTING_RUNTIME_BARE_METAL --copt=-DROO_TESTING_RUNTIME_FREERTOS=1
expect_failure "runtime identity is mandatory" \
  'Select exactly one roo_testing application runtime' \
  run_generic build //roo_testing/frameworks/environment:runtime \
  --copt=-UROO_TESTING_RUNTIME_BARE_METAL
expect_failure "runtime macro values must be canonical" \
  'Compiler runtime identity does not match the bare-metal platform' \
  run_generic build //roo_testing/frameworks/environment:runtime \
  --copt=-UROO_TESTING_RUNTIME_BARE_METAL --copt=-DROO_TESTING_RUNTIME_BARE_METAL=0
expect_failure "runtime platform constraint is mandatory" \
  'incompatible' \
  run_generic build //roo_testing/frameworks/environment:runtime \
  --platforms=//test/profile:incorrect_arduino_soc
expect_failure "typed SoC identities cannot be mixed" \
  'Select exactly one supported roo_testing SoC' \
  run_generic build //roo_testing/soc:identity --copt=-DROO_TESTING_SOC_ESP32C3=1
expect_failure "generic typed identity agrees with its platform" \
  'Typed identity does not match selected SoC platform' \
  run_generic build //roo_testing/soc:identity \
  --copt=-UROO_TESTING_SOC_ESP32 --copt=-DROO_TESTING_SOC_ESP32C3=1 \
  --copt=-UROO_TESTING_SOC '--copt=-DROO_TESTING_SOC="esp32c3"'
expect_failure "SoC name agrees with typed identity" \
  'static assertion failed' \
  run_generic build //roo_testing/soc:identity \
  --copt=-UROO_TESTING_SOC '--copt=-DROO_TESTING_SOC="esp32c3"'

for soc in esp32 esp32s3 esp32c3; do
  for frontend in arduino idf; do
    profile="--config=roo_testing_${frontend}_${soc}"
    environment_test=global_environment_test
    select_test=arduino_select_test
    contract=arduino_esp32
    if [[ "${frontend}" == idf ]]; then
      environment_test=idf_global_environment_test
      select_test=idf_select_test
      contract=idf
    fi
    run_bazel test "${profile}" \
      "//test/profile:${environment_test}" "//test/profile:${select_test}" \
      //test/profile:runtime_selection_test //test:soc_profile_test \
      --test_output=errors
    run_bazel build "${profile}" \
      "//roo_testing/frameworks/environment:${contract}" //roo_testing/soc:target
    expect_failure "${soc}/${frontend} rejects a bare-metal compiler identity" \
      'Compiler runtime identity does not match the FreeRTOS platform' \
      run_bazel build "${profile}" //roo_testing/frameworks/environment:runtime \
      --copt=-UROO_TESTING_RUNTIME_FREERTOS --copt=-DROO_TESTING_RUNTIME_BARE_METAL=1
    expect_failure "${soc}/${frontend} retains IDF target validation" \
      'Select exactly one supported ESP-IDF target' \
      run_bazel build "${profile}" //roo_testing/soc:target \
      "--copt=-UCONFIG_IDF_TARGET_${soc^^}"
    for example in gpio onewire rtc_ds3231_i2c simple tft_display tft_touch; do
      cmp ".roo_testing/bazelrc/${soc}/base.bazelrc" \
        "examples/${example}/.roo_testing/bazelrc/${soc}/base.bazelrc"
    done
  done
done

echo "roo_testing generic identity, runtime, and six existing profiles verified"
