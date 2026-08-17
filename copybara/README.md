# Framework imports

Copybara owns the upstream portions of these stable source trees:

- `roo_testing/frameworks/arduino-esp32`
- `roo_testing/frameworks/esp-idf`

The current pins are Arduino-ESP32 3.3.11 at
`189089bb76e74978dc95abebefdad42b0d421ba9` and ESP-IDF 6.0.2 at
`7101770dc6db2667b3c477cc31365dd1acd6db4e`. Arduino-ESP32 3.3.11 declares
compatibility with ESP-IDF versions from 5.3 up to (but not including) 6.2,
so 6.0.2 is the newest stable ESP-IDF release in its supported range.

## Ownership boundary

The import is intentionally narrower than either upstream repository. It keeps
the ESP32 core, the Arduino libraries exposed by roo_testing, the default ESP32
variant, and the ESP-IDF components required by those APIs. Upstream examples,
tests, bulk documentation trees, precompiled target artifacts, and source
dedicated to other Espressif chips are excluded in `copy.bara.sky`.

Bazel `BUILD`, `BUILD.bazel`, and `.bzl` files under the imported trees are
destination-owned overlays. The host `sdkconfig.h` and deliberately named
`roo_testing_*` bridge files are protected the same way. Copybara's
`destination_files` filters preserve them across updates. Host implementations
should otherwise live in `roo_testing/frameworks/esp32_shims`; if an upstream
source file must change, record that delta in the applicable patch series
instead of editing an imported file without a patch.

ESP-IDF embeds many Git submodules. The workflow fetches only the lwIP and
SPIFFS source submodules and explicitly excludes target binary libraries and
unrelated third-party stacks.

## Prerequisites

Install a current Copybara weekly release and Java 21 or newer. Either put the
`copybara` executable on `PATH`, set `COPYBARA_BIN` to its path, or set
`COPYBARA_JAR` to a Copybara deploy jar. Git credentials are not needed when
using `import.sh`, because it overrides the destination with the current local
repository.

Validate the configuration without checking out either upstream repository:

```sh
copybara/import.sh validate
```

## Running an import

Imports require a clean worktree. The helper targets the currently checked-out
branch and Copybara creates a normal squash commit there. Review a prospective
update first:

```sh
copybara/import.sh --dry-run esp-idf
copybara/import.sh --dry-run arduino-esp32
```

Apply one or both workflows after reviewing the diff:

```sh
copybara/import.sh esp-idf
copybara/import.sh arduino-esp32
copybara/import.sh all
```

The first Copybara import into a repository that has no origin-revision label
may need `--force` once to establish its baseline:

```sh
copybara/import.sh --dry-run --force all
copybara/import.sh --force all
```

`all` imports ESP-IDF before Arduino so the framework pair is committed in
dependency order. Each workflow has its own origin revision label, so future
runs import only changes since that framework's previous revision.

## Updating the framework pair

1. Choose an Arduino-ESP32 release and the ESP-IDF release line declared by
   that Arduino release.
2. Resolve both tags to immutable commits and update `versions.bara.sky`.
3. If new APIs require more source, extend `ARDUINO_LIBRARIES` or
   `ESP_IDF_COMPONENTS` deliberately. Do not broaden the import to the complete
   upstream repositories.
4. Refresh patches against the new upstream revisions and list them in their
   `series` files.
5. Run both workflows with `--dry-run`, inspect all additions and deletions,
   then run the real imports.
6. Build and test roo_testing and its dependent libraries before publishing the
   update.

An explicit revision may also be supplied as the final Copybara argument when
running Copybara directly. Normal updates should change the reviewed pins in
`versions.bara.sky`, ensuring that everyone reproduces the same source trees.

## Patch series

Patch paths are relative to this directory and use standard Git patches with
`a/` and `b/` prefixes. Each non-comment line in a `series` file names one patch
relative to that series file. Patches apply before Copybara moves the upstream
checkout into its stable destination.

Keep patches small and ordered by abstraction: compatibility fixes first,
behavioral shims second. A typical refresh starts from a clean checkout at the
pinned revision, reapplies the roo_testing change, and captures it with
`git diff --binary --full-index`. Empty patch series are valid.

Do not put Bazel overlays in patches. BUILD files remain ordinary destination
files so they can evolve independently of upstream source and survive every
Copybara refresh.
