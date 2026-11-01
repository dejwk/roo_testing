#!/usr/bin/env python3
"""Validate supported Pico consumers using coordinated local Roo checkouts."""

import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CLIENT_ROOT = Path(os.environ.get("ROO_CLIENT_ROOT", ROOT.parent)).resolve()
TEST_ROOT = Path(os.environ.get("ROO_TESTING_PROFILE_TEST_ROOT",
                              Path.home() / ".cache/roo_testing/profile-integration")).resolve()
WORKSPACE = TEST_ROOT / "clients"


def Run(command, *args, profile=None):
    """Serialize client builds under the same persistent output-user root."""
    invocation = [os.environ.get("BAZEL", "bazel"),
                  f"--output_user_root={TEST_ROOT / 'configured'}", command]
    if profile is not None:
        invocation.append(f"--config=roo_testing_{profile}")
    invocation.extend(args)
    result = subprocess.run(invocation, cwd=WORKSPACE, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="", flush=True)
    return result.stdout


def Setup():
    """Materialize a stable client workspace without editing client manifests."""
    if TEST_ROOT == Path("/tmp") or Path("/tmp") in TEST_ROOT.parents:
        raise RuntimeError("Refusing RAM-backed Bazel storage")
    modules = ("roo_threads", "roo_time", "roo_scheduler", "roo_display",
               "roo_io", "roo_logging", "roo_collections", "roo_backport")
    paths = {name: CLIENT_ROOT / name for name in modules}
    paths["roo_testing"] = ROOT
    for name, path in paths.items():
        if not (path / "MODULE.bazel").is_file():
            raise RuntimeError(f"Missing local {name}: {path}; set ROO_CLIENT_ROOT")
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    manifest = '''module(name = "pico_clients")
bazel_dep(name = "roo_testing", version = "2.4.0")
bazel_dep(name = "roo_threads", version = "1.2.9")
bazel_dep(name = "roo_time", version = "2.0.2")
bazel_dep(name = "roo_scheduler", version = "2.3.0")
bazel_dep(name = "roo_display", version = "3.4.0")
bazel_dep(name = "rules_cc", version = "0.2.25")
bazel_dep(name = "googletest", version = "1.18.0.bcr.1")
'''
    for name, path in paths.items():
        manifest += f"local_path_override(module_name = {json.dumps(name)}, path = {json.dumps(str(path))})\n"
    (WORKSPACE / "MODULE.bazel").write_text(manifest)
    (WORKSPACE / "BUILD").write_text("")
    shutil.copytree(ROOT / ".roo_testing/bazelrc", WORKSPACE / ".roo_testing/bazelrc",
                    dirs_exist_ok=True)
    for name in (".bazelrc", ".bazelversion"):
        shutil.copyfile(ROOT / name, WORKSPACE / name)


def CheckIsolation(profile, targets):
    """Ensure supported clients do not reintroduce the ESP32 runtime."""
    labels = Run("cquery", "deps(set(" + " ".join(targets) + "))", profile=profile)
    for forbidden in ("frameworks/esp-idf", "frameworks/arduino-esp32",
                      "microcontrollers/esp32", "roo_testing/sys:"):
        if forbidden in labels:
            raise RuntimeError(f"Pico clients acquired {forbidden}")
    (TEST_ROOT / f"clients-{profile}.deps").write_text(labels)


def Main():
    """Check time, single-thread locks, cooperative scheduling and display I/O."""
    Setup()
    common = ["@roo_threads//:backend_selection_test", "@roo_time//:roo_time_test",
              "@roo_time//:compact_time_test", "@roo_time//:format_test",
              "@roo_time//:format_portable_test"]
    arduino = common + ["@roo_scheduler//:roo_scheduler_test",
                        "@roo_scheduler//:roo_scheduler_allocation_test",
                        "@roo_scheduler//:roo_scheduler_auto_time_test",
                        "@roo_scheduler//:bare_metal_dispatch_test",
                        "@roo_display//:pico_spi_transport_test"]
    try:
        for profile, targets in (("pico_rp2040", common), ("arduino_rp2040", arduino)):
            Run("test", *targets, "--test_output=errors", profile=profile)
            CheckIsolation(profile, targets)
            Run("test", *targets, "--config=asan", "--test_output=errors", profile=profile)
        Run("build", "@roo_display//:arduino_spi_compile", profile="arduino_rp2040")
        Run("build", "@roo_display//:arduino_spi_compile", profile="arduino_esp32")
        Run("test", "@roo_time//:format_test", "@roo_scheduler//:roo_scheduler_test",
            "@roo_scheduler//:roo_scheduler_concurrency_test",
            "@roo_threads//:backend_selection_test", "--test_output=errors",
            profile="arduino_esp32")
        print("Supported Pico clients, isolated dependencies, ASAN and ESP32 regressions verified")
    finally:
        # Do not leave an additional client-workspace Bazel server resident.
        Run("shutdown")


if __name__ == "__main__":
    Main()
