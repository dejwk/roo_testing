# Build-profile integration tests

The Arduino and IDF global-environment tests compile unrelated C and C++
libraries with no roo_testing dependency and verify their exact macro sets.
The IDF test additionally rejects every Arduino/board macro. Frontend tests
prove the shared ESP-IDF capability and mutually exclusive `is_idf` and
`is_arduino` selections. IDF smoke tests exercise the stable
`//:esp_idf_main` and `//:esp_idf_gtest_main` entry points.

The Arduino positives run in the normal root suite. IDF positives are manual so
the Arduino-default wildcard does not analyze them. Run both frontend matrices,
configuration-boundary checks, composition probes, and action-command-line
checks with:

```sh
./test/profile/verify_profile.sh
```

The runner uses `$HOME/.cache/roo_testing/profile-integration` by default and
rejects `/tmp` for every configurable Bazel storage path. It verifies expected
failures for a plain host platform, an unsupported SoC, and a selected
roo_testing platform whose compiler profile is absent. It also builds explicit
user-`--copt` and ASAN probes and checks with `aquery` that every canonical
definition occurs exactly once in unrelated C and C++ compile actions.
It preserves the user's home rc (and its disk cache), suppressing only the
roo_testing workspace rc when exercising the mutually exclusive IDF profile.
