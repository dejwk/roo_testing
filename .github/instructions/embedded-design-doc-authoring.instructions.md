---
name: "Embedded Design Doc Authoring"
description: "Use when writing or updating design docs, implementation plans, or API proposals in this repository. Shared baseline across roo libraries."
applyTo:
  - "docs/**/*.md"
  - "doc/**/*.md"
---
# Embedded Design Doc Authoring

Use this instruction for shared design-doc expectations across roo
repositories. Repo-local guidance can add project-specific references,
validation, and constraints on top of this baseline.

## Audience and Purpose

A design document serves three audiences at once:

- agents and humans who will implement the design;
- human reviewers who must evaluate its correctness and tradeoffs; and
- future maintainers who use it as durable project documentation.

Write so a human reader can understand the problem, vocabulary, decisions, and
rationale without reconstructing them from the proposed API or implementation
plan. Implementation precision does not replace explanatory clarity.

## Required Structure

Use this section order unless a narrower document genuinely needs less:

1. Objective
2. Motivation
3. Background
4. Requirements
5. Design Overview
6. Design Details
7. Proposed API
8. Implementation Plan
9. Testing Plan
10. Caveats
11. Future Work (optional)

## Writing Rules

- Be succinct.
- Keep Objective limited to the intended outcome. Define terms in Background.
- Define current concepts in Background and proposed concepts in Design
  Overview before later sections rely on them.
- State Requirements in problem-domain language, independent of proposed API
  names and mechanics.
- Prefer Markdown hyperlinks for stable cross-references.
- Do not repeat the same content across sections.
- Keep Motivation brief; put detailed enumeration in Requirements.
- Put major decisions in Design Overview and mechanics in Design Details.
- In Design Overview, explicitly map the solution to the Requirements.
- Explain the behavioral or lifetime need for a data structure before its
  mechanics.
- Split Implementation Plan into small phases that each map to one commit.
- Start Implementation Plan with a link to repo-local code-authoring guidance
  when one exists.
- Give every phase a proposed commit message and narrow completion validation.
- Include a feature's tests and documentation in the phase that adds it.
- Keep Testing Plan as a summary; do not repeat phase-level test cases.
- Put rejected alternatives under `### Rejected Alternatives` in Caveats.
- Use Future Work only for intentionally out-of-scope improvements.
- LaTeX math is acceptable when it clarifies technical or cost analysis.
- For APIs that land before full support, define interim behavior explicitly:
  return an error where possible; otherwise log an unimplemented warning and
  use a safe degenerate fallback, or log fatally when no safe fallback exists.

## Closing On Decisions

A design document closes decisions rather than merely enumerating them. Resolve
every open question or turn it into a numbered implementation experiment with
success criteria.

- Replace hedged phrasing with a chosen option and its reasoning.
- Quantitative tradeoffs include the analysis used to select an option.
- When analysis cannot close a choice, add a targeted measurement phase with a
  defined exit criterion.
- Record rejected alternatives in Caveats and point back to the deciding
  section.
- Do not defer anything required for correctness to Future Work.
- Re-read the finished document and remove wording that hides an unresolved
  choice.

## Checklist

- Section order matches the required structure.
- Stable references are hyperlinks.
- Implementers, reviewers, and maintainers can understand the document without
  reverse-engineering terminology from code.
- Objective is succinct; Background defines current concepts; Design Overview
  defines proposed concepts; Requirements use problem-domain language.
- Design Overview maps solution elements to requirements.
- Nontrivial data structures and ownership links are motivated.
- Implementation phases are incremental, testable, and single-commit, with a
  proposed message and validation.
- Functionality, its tests, and its documentation land together.
- Decisions are closed and substantial rejected alternatives are recorded.
- Future Work is genuinely optional.
- Partially implemented APIs define explicit interim behavior.
- Testing Plan summarizes rather than repeats implementation details.
