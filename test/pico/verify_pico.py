#!/usr/bin/env python3
"""Validate the SDK-only native profile and its framework isolation."""

import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
TEST_ROOT = Path(os.environ.get(
    "ROO_TESTING_PROFILE_TEST_ROOT",
    str(Path.home() / ".cache/roo_testing/profile-integration"),
)).resolve()
PROFILE = "--config=roo_testing_pico_rp2040"


def RunBazel(command, *args, profile=PROFILE, succeeds=True, diagnostic=None):
    """Run one Bazel command in the workspace's persistent validation root."""
    invocation = [os.environ.get("BAZEL", "bazel"),
                  f"--output_user_root={TEST_ROOT / 'configured'}", command]
    if profile is not None:
        invocation.append(profile)
    if not succeeds:
        invocation.append("--keep_going")
    invocation.extend(args)
    result = subprocess.run(invocation, cwd=ROOT, text=True, capture_output=True)
    if succeeds:
        if result.returncode != 0:
            raise RuntimeError(result.stdout + result.stderr)
    elif result.returncode == 0 or diagnostic not in result.stdout + result.stderr:
        raise RuntimeError(f"Expected failure containing {diagnostic!r}:\n"
                           + result.stdout + result.stderr)
    return result.stdout


def CheckIsolation():
    """Inspect configured dependencies and the actual native compile commands."""
    query = "deps(set(//test/pico:sdk_probe_c //test/pico:sdk_probe_cpp))"
    labels = RunBazel("cquery", query)
    for forbidden in ("frameworks/esp-idf", "frameworks/arduino-esp32",
                      "frameworks/arduino-pico", "frameworks/esp32_shims",
                      "microcontrollers/esp32", "roo_testing/sys:"):
        if forbidden in labels:
            raise RuntimeError(f"Pico acquired forbidden dependency: {forbidden}")
    (TEST_ROOT / "pico-stage2.deps").write_text(labels)
    raw = RunBazel("aquery", query, "--output=jsonproto", "--include_commandline")
    (TEST_ROOT / "pico-stage2.aquery.json").write_text(raw)
    required = ("ROO_TESTING", "ROO_TESTING_SOC", "ROO_TESTING_SOC_RP2040",
                "ROO_TESTING_RUNTIME_BARE_METAL", "PICO_BUILD", "PICO_ON_DEVICE",
                "PICO_NO_HARDWARE", "PICO_RP2040", "PICO_RP2350",
                "PICO_PLATFORM_STRING", "PICO_BOARD")
    sdk_sources = set()
    actions = json.loads(raw)["actions"]
    for action in actions:
        if action["mnemonic"] != "CppCompile":
            continue
        args = action["arguments"]
        for definition in required:
            count = sum(arg.startswith(f"-D{definition}=") for arg in args)
            if count != 1:
                raise RuntimeError(f"Expected one global {definition} in {args}")
        for arg in args:
            if arg.startswith(("-DARDUINO", "-DESP_PLATFORM", "-DESP32",
                               "-DCONFIG_IDF", "-D__arm__", "-D__riscv")):
                raise RuntimeError(f"Contradictory native compile identity: {arg}")
        source = args[args.index("-c") + 1]
        if source.endswith((".S", ".s", ".asm")):
            raise RuntimeError(f"Device assembly entered the native build: {source}")
        if "/frameworks/pico-sdk/" in source:
            sdk_sources.add(source.split("/frameworks/pico-sdk/", 1)[1])
    if sdk_sources != {"src/host/hardware_timer/timer.c",
                       "src/host/pico_platform/platform_base.c"}:
        raise RuntimeError(f"Unexpected SDK implementation sources: {sdk_sources}")


def CheckBinary():
    """Check native ELF identity, one SDK implementation, and explicit failure."""
    output = RunBazel("cquery", "//test/pico:sdk_probe_cpp", "--output=files")
    binary = ROOT / next(line for line in output.splitlines()
                         if line.endswith("/sdk_probe_cpp"))
    elf = binary.read_bytes()
    native = Path("/proc/self/exe").read_bytes()
    if elf[:4] != b"\x7fELF" or elf[4:6] != native[4:6] or elf[18:20] != native[18:20]:
        raise RuntimeError("Probe ELF ABI/CPU does not match the native execution host")
    symbols = subprocess.check_output(
        ["nm", "-g", "--defined-only", str(binary.with_name("sdk_probe_c"))],
        text=True)
    names = [line.split()[-1] for line in symbols.splitlines() if line.split()]
    for symbol in ("time_us_32", "time_us_64", "get_core_num"):
        if names.count(symbol) != 1:
            raise RuntimeError(f"Expected one linked implementation of {symbol}")
    if any(name.startswith(("esp_", "vTask")) for name in names):
        raise RuntimeError("ESP-IDF/FreeRTOS symbols entered the native binary")
    # Disable core dumps for the expected upstream panic trap.
    def DisableCoreDump():
        import resource
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    failure = subprocess.run([str(binary), "unsupported-alarm"], capture_output=True,
                             preexec_fn=DisableCoreDump)
    if failure.returncode >= 0:
        raise RuntimeError("Unsupported hardware alarm must terminate explicitly")


def CheckFailures():
    """Require backend guards and compile contracts to reject mixed profiles."""
    for profile in ("--config=roo_testing_arduino_esp32",
                    "--config=roo_testing_idf_esp32"):
        RunBazel("build", "//:pico_sdk", profile=profile, succeeds=False,
                 diagnostic="requires target-platform constraint(s)")
    for target in ("//:arduino", "//:esp_idf_main"):
        RunBazel("build", target, succeeds=False,
                 diagnostic="requires target-platform constraint(s)")
    for option, diagnostic in (
        ("--copt=-DARDUINO=10819", "SDK-only Pico frontend must not carry Arduino identity"),
        ("--copt=-DPICO_ON_DEVICE=1", "Pico SDK requires native host platform identity"),
        ("--copt=-DESP_PLATFORM=1", "Pico SDK profiles must not carry ESP-IDF identity"),
        ("--copt=-DROO_TESTING_SOC_ESP32=1", "Select exactly one supported roo_testing SoC"),
        ("--copt=-UROO_TESTING_RUNTIME_BARE_METAL", "Select exactly one roo_testing application runtime"),
    ):
        RunBazel("build", "//:pico_sdk", option, succeeds=False, diagnostic=diagnostic)
    RunBazel("build", "//test/pico:unsupported_sleep_probe", succeeds=False,
             diagnostic="undefined reference to `sleep_us'")


def Main():
    """Run the stage's probes before wider profile regressions."""
    if TEST_ROOT == Path("/tmp") or Path("/tmp") in TEST_ROOT.parents:
        raise RuntimeError(f"Refusing RAM-backed Bazel storage: {TEST_ROOT}")
    TEST_ROOT.mkdir(parents=True, exist_ok=True)
    print(RunBazel("test", "//:pico_tests", "--test_output=errors"), end="")
    # Cached tests need not materialize their executables after another profile
    # reused the output paths. Run the probes before inspecting their ELF files.
    for target in ("//test/pico:sdk_probe_c", "//test/pico:sdk_probe_cpp"):
        print(RunBazel("run", target), end="")
    CheckIsolation()
    CheckBinary()
    CheckFailures()
    print(RunBazel("test", "//:pico_tests", "--config=asan", "--test_output=errors"), end="")
    print("Pico SDK native probes, isolation, explicit failures, and ASAN verified")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
