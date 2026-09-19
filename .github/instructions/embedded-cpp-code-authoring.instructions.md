---
name: "Embedded C++ Code Authoring"
description: "Use when editing embedded C++ library code, public APIs, tests, or validation targets in this repository. Shared baseline across roo libraries."
applyTo:
  - "**/*.c"
  - "**/*.cc"
  - "**/*.cpp"
  - "**/*.h"
  - "**/*.hh"
  - "**/*.hpp"
  - "**/*.ino"
  - "**/*.bzl"
  - "BUILD"
  - "MODULE.bazel"
---
# Embedded C++ Code Authoring

Use this instruction for shared code-authoring expectations across roo
repositories. Repo-local guidance can add repository-specific validation and
policy on top of this baseline.

## Core Conventions

- Use braces for control-flow bodies unless the entire construct, including
  its condition and single-statement body, fits on one line within the style's
  column limit (for example, `if (ptr == nullptr) return;`). Multiline constructs
  require braces. Format C++ with the Google baseline; do not force
  `InsertBraces: true`, since single-line unbraced constructs are permitted.
- Do not rely on implicit or contextual conversions to `bool`. Compare pointers
  and smart pointers explicitly with `nullptr`, numeric values and counts with
  zero, and bitmasks with zero. This applies to conditions, logical operators,
  and conditional (`?:`) expressions. Boolean values and predicates may be used
  directly; do not add redundant `== true` or `== false` comparisons.
- Follow Google-style C++, except instance methods use `camelCase()`.
  Trivial accessors and mutators may keep `snake_case()` when that reads more
  naturally and matches their fields.
- Favor readability. Avoid redundant branches, repeated explanations, and
  unnecessary line count.
- Keep `CHECK` and related assertion macros at their point of use so failures
  report the source line that expresses the violated contract.
- Embedded-target code must build with exceptions disabled (`-fno-exceptions`);
  do not use `throw`, `try`, `catch`, or exception-dependent behavior.
- Avoid `const_cast` as a way to bridge const/non-const mismatches; fix the
  interface unless the target is provably non-mutating on that path.
- Avoid RTTI-dependent constructs such as `dynamic_cast` and `typeid` in
  embedded-target code.
- Avoid long lambdas. Put substantial logic in an unnamed-namespace helper.
- Avoid `auto` unless its type is obvious or spelling it is excessively complex.
- Be conservative about RAM. Prefer shared data, existing ownership points, and
  zero-cost hooks over per-instance state.
- Use `///` for Doxygen comments; do not use block-form Doxygen comments.
- All public classes and public methods have Doxygen comments at declaration.
- Leave one empty separator line between public methods or functions that have
  Doxygen comments, including one-line declarations. Adjacent undocumented
  one-line declarations may omit the separator.
- Always leave one empty separator line between adjacent `struct` or `class`
  declarations.
- Doxygen describes implemented behavior, or the contract for pure virtual and
  otherwise contract-defining declarations.
- Every code change ships with focused unit tests.
- Non-trivial tests have a brief `Verifies ...` comment immediately before the
  test declaration.
- Keep comments sparse. Complex algorithms explain their main decisions and
  important branches rather than restating mechanics.
- Non-trivial helpers and methods have a short summary of what they compute,
  classify, or guarantee.

## Design-Stage Commits

- When a change implements one design-document stage, include a proposed commit
  message in the completion note even if no commit is created.
- Use a two-part structure: one summary sentence followed by one descriptive
  paragraph.
- The summary starts with the design document and stage, then states what
  landed.
- The paragraph explains the concrete landed slice, names the API/tests/docs
  or validation added, and references the relevant design document.
- Treat the design's proposed commit message as the starting point unless the
  implemented slice differs.

## Validation

- Prefer the narrowest relevant test, build, or typecheck target first, then
  widen only when needed.
- Use repository-local guidance to find validation commands and integration
  builds.
- Before handoff, run `clang-format` on every changed C++ source and header.

## Checklist

- Public API declarations have `///` Doxygen comments.
- Documented public methods and functions have empty separator lines between
  their declarations.
- Adjacent `struct` and `class` declarations have empty separator lines.
- The code change includes focused unit tests with `Verifies ...` comments.
- Validation starts with the narrowest relevant target.
- `clang-format` has run on every changed C++ source and header.
- Complex code explains its strategy and important branches.
- The change avoids avoidable per-instance RAM cost.
- The completion note for a design stage includes a standalone proposed commit
  message, descriptive paragraph, and design-document reference.
