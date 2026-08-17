#!/usr/bin/env bash
set -euo pipefail

# The same file acts as the fake Bazel executable used by the tests below.
if [[ "${ROO_TESTING_FAKE_BAZEL:-}" == "1" ]]; then
  capture="${ROO_TESTING_FAKE_CAPTURE:?missing fake capture path}"
  {
    printf 'call\n'
    printf 'skip=%s\n' "${BAZELISK_SKIP_WRAPPER:-}"
    for arg in "$@"; do
      printf 'arg=%s\n' "${arg}"
    done
    printf 'end\n'
  } >>"${capture}"

  for arg in "$@"; do
    if [[ -n "${ROO_TESTING_FAKE_FAIL_CONFIG:-}" &&
          "${arg}" == "--config=${ROO_TESTING_FAKE_FAIL_CONFIG}" ]]; then
      exit "${ROO_TESTING_FAKE_FAIL_STATUS:-1}"
    fi
  done
  exit 0
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
workspace="$(realpath "${script_dir}/../..")"
wrapper="${workspace}/.roo_testing/bin/bazel"
profile_helper="${workspace}/.roo_testing/bin/test_all_profiles"
test_root="${ROO_TESTING_WRAPPER_TEST_ROOT:-${HOME}/.cache/roo_testing/wrapper-integration}"

resolved_test_root="$(realpath -m "${test_root}")"
if [[ "${resolved_test_root}" == /tmp || "${resolved_test_root}" == /tmp/* ]]; then
  echo "Refusing RAM-backed test path: ${test_root}" >&2
  exit 1
fi

mkdir -p "${test_root}"
scratch="$(mktemp -d "${test_root}/case.XXXXXX")"
trap 'rm -rf -- "${scratch}"' EXIT
capture="${scratch}/capture"
stderr_log="${scratch}/stderr"
self="$(realpath "${BASH_SOURCE[0]}")"

fail() {
  echo "bazel wrapper test failed: $*" >&2
  exit 1
}

assert_lines() {
  local -a expected=("$@")
  local -a actual=()
  mapfile -t actual <"${capture}"
  if ((${#actual[@]} != ${#expected[@]})); then
    printf >&2 'expected lines:\n  %s\n' "${expected[*]}"
    printf >&2 'actual lines:\n  %s\n' "${actual[*]}"
    fail "capture line count differs"
  fi
  for ((i = 0; i < ${#expected[@]}; ++i)); do
    if [[ "${actual[i]}" != "${expected[i]}" ]]; then
      fail "line ${i}: expected '${expected[i]}', got '${actual[i]}'"
    fi
  done
}

invoke_wrapper() {
  : >"${capture}"
  : >"${stderr_log}"
  env -u BAZELISK_SKIP_WRAPPER \
    BAZEL_REAL="${self}" \
    ROO_TESTING_FAKE_BAZEL=1 \
    ROO_TESTING_FAKE_CAPTURE="${capture}" \
    "${wrapper}" "$@" 2>"${stderr_log}"
}

default_config="--config=roo_testing_arduino_esp32"
for command in \
    aquery build coverage cquery fetch info mobile-install print_action run test vendor; do
  invoke_wrapper "${command}" //:target
  assert_lines \
    call skip= "arg=${command}" "arg=${default_config}" arg=//:target end
  grep -Fq 'defaulting to --config=roo_testing_arduino_esp32' "${stderr_log}" ||
    fail "${command} did not announce the Arduino default"
done

invoke_wrapper build --config=roo_testing_idf_esp32s3 //:target
assert_lines \
  call skip= arg=build arg=--config=roo_testing_idf_esp32s3 arg=//:target end
[[ ! -s "${stderr_log}" ]] || fail "explicit future IDF profile printed a default notice"

invoke_wrapper test --config roo_testing_arduino_esp32 //...
assert_lines \
  call skip= arg=test arg=--config arg=roo_testing_arduino_esp32 arg=//... end
[[ ! -s "${stderr_log}" ]] || fail "explicit Arduino profile printed a default notice"

invoke_wrapper run //:app -- --config=roo_testing_idf_esp32
assert_lines \
  call skip= arg=run "arg=${default_config}" arg=//:app arg=-- \
  arg=--config=roo_testing_idf_esp32 end

invoke_wrapper --output_base build test //...
assert_lines \
  call skip= arg=--output_base arg=build arg=test "arg=${default_config}" arg=//... end

for startup_option in \
    --command_port \
    --host_jvm_profile \
    --install_base \
    --invocation_policy \
    --unix_digest_hash_attribute_name; do
  invoke_wrapper "${startup_option}" build test //...
  assert_lines \
    call skip= "arg=${startup_option}" arg=build arg=test \
    "arg=${default_config}" arg=//... end
done

for test_case in \
    'build --help' \
    'help build' \
    'config build' \
    '--noworkspace_rc build //:target' \
    '--ignore_all_rc_files build //:target'; do
  read -r -a case_args <<<"${test_case}"
  invoke_wrapper "${case_args[@]}"
  expected=(call skip=)
  for arg in "${case_args[@]}"; do
    expected+=("arg=${arg}")
  done
  expected+=(end)
  assert_lines "${expected[@]}"
  [[ ! -s "${stderr_log}" ]] || fail "unexpected notice for ${test_case}"
done

invoke_wrapper --noworkspace_rc --workspace_rc build //:target
assert_lines \
  call skip= arg=--noworkspace_rc arg=--workspace_rc arg=build \
  "arg=${default_config}" arg=//:target end

invoke_wrapper --ignore_all_rc_files --noignore_all_rc_files build //:target
assert_lines \
  call skip= arg=--ignore_all_rc_files arg=--noignore_all_rc_files arg=build \
  "arg=${default_config}" arg=//:target end

invoke_wrapper --workspace_rc=false --workspace_rc=true build //:target
assert_lines \
  call skip= arg=--workspace_rc=false arg=--workspace_rc=true arg=build \
  "arg=${default_config}" arg=//:target end

invoke_wrapper --ignore_all_rc_files=true --ignore_all_rc_files=false \
  build //:target
assert_lines \
  call skip= arg=--ignore_all_rc_files=true arg=--ignore_all_rc_files=false \
  arg=build "arg=${default_config}" arg=//:target end

invoke_wrapper --quiet build //:target
assert_lines \
  call skip= arg=--quiet arg=build "arg=${default_config}" arg=//:target end
[[ ! -s "${stderr_log}" ]] || fail "--quiet did not suppress the default notice"

invoke_wrapper --quiet=true --quiet=false build //:target
assert_lines \
  call skip= arg=--quiet=true arg=--quiet=false arg=build \
  "arg=${default_config}" arg=//:target end
grep -Fq 'defaulting to --config=roo_testing_arduino_esp32' "${stderr_log}" ||
  fail "last --quiet=false did not restore the default notice"

invoke_wrapper build \
  --config=roo_testing_arduino_esp32 \
  --config=roo_testing_arduino_esp32 //:target
assert_lines \
  call skip= arg=build \
  arg=--config=roo_testing_arduino_esp32 \
  arg=--config=roo_testing_arduino_esp32 arg=//:target end
[[ ! -s "${stderr_log}" ]] || fail "duplicate identical profile printed a notice"

: >"${capture}"
: >"${stderr_log}"
set +e
env -u BAZELISK_SKIP_WRAPPER \
  BAZEL_REAL="${self}" \
  ROO_TESTING_FAKE_BAZEL=1 \
  ROO_TESTING_FAKE_CAPTURE="${capture}" \
  "${wrapper}" build \
    --config=roo_testing_arduino_esp32 \
    --config=roo_testing_idf_esp32 //:target 2>"${stderr_log}"
status=$?
set -e
[[ ${status} -eq 2 ]] || fail "mixed profiles returned ${status}, expected 2"
[[ ! -s "${capture}" ]] || fail "mixed profiles reached Bazel"
grep -Fq 'select exactly one frontend/SoC profile' "${stderr_log}" ||
  fail "mixed profiles did not explain the error"

: >"${capture}"
: >"${stderr_log}"
set +e
BAZEL_REAL="${self}" \
ROO_TESTING_FAKE_BAZEL=1 \
ROO_TESTING_FAKE_CAPTURE="${capture}" \
  "${wrapper}" build \
    --config=roo_testing_arduino_esp32 \
    --config=roo_testing_arduino_esp32s3 //:target 2>"${stderr_log}"
status=$?
set -e
[[ ${status} -eq 2 ]] || fail "distinct Arduino profiles returned ${status}, expected 2"
[[ ! -s "${capture}" ]] || fail "distinct Arduino profiles reached Bazel"
grep -Fq 'select exactly one frontend/SoC profile' "${stderr_log}" ||
  fail "distinct Arduino profiles did not explain the error"

# Direct invocation takes the recursion-safe fallback and marks the nested
# Bazelisk invocation so that it will not rediscover the wrapper.
ln -s "${self}" "${scratch}/bazel"
: >"${capture}"
env -u BAZEL_REAL \
  PATH="${scratch}:${PATH}" \
  ROO_TESTING_FAKE_BAZEL=1 \
  ROO_TESTING_FAKE_CAPTURE="${capture}" \
  "${wrapper}" version
assert_lines call skip=true arg=version end

# The two-profile helper forwards every argument, selects profiles explicitly,
# and stops immediately when the first invocation fails.
: >"${capture}"
: >"${stderr_log}"
ROO_TESTING_BAZEL="${self}" \
ROO_TESTING_FAKE_BAZEL=1 \
ROO_TESTING_FAKE_CAPTURE="${capture}" \
  "${profile_helper}" //... --test_output=errors 2>"${stderr_log}"
assert_lines \
  call skip= arg=test arg=--config=roo_testing_arduino_esp32 arg=//... \
  arg=--test_output=errors end \
  call skip= arg=test arg=--config=roo_testing_idf_esp32 arg=//... \
  arg=--test_output=errors end
grep -Fq 'testing the Arduino ESP32 profile' "${stderr_log}" ||
  fail "helper omitted Arduino notice"
grep -Fq 'testing the ESP-IDF ESP32 profile' "${stderr_log}" ||
  fail "helper omitted IDF notice"

: >"${capture}"
set +e
ROO_TESTING_BAZEL="${self}" \
ROO_TESTING_FAKE_BAZEL=1 \
ROO_TESTING_FAKE_CAPTURE="${capture}" \
ROO_TESTING_FAKE_FAIL_CONFIG=roo_testing_arduino_esp32 \
ROO_TESTING_FAKE_FAIL_STATUS=17 \
  "${profile_helper}" //... >/dev/null 2>"${stderr_log}"
status=$?
set -e
[[ ${status} -eq 17 ]] || fail "helper returned ${status}, expected 17"
assert_lines \
  call skip= arg=test arg=--config=roo_testing_arduino_esp32 arg=//... end

grep -Fxq 'BAZELISK_WRAPPER_DIRECTORY=.roo_testing/bin' \
  "${workspace}/.bazeliskrc" || fail "root .bazeliskrc does not select the wrapper"

echo "roo_testing Bazel wrapper and profile helper verified"
