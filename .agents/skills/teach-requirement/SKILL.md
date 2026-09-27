---
name: teach-requirement
description: Explain an AdaptivePi repository requirement in plain language and teach how to implement and verify it. Use when the user asks about a requirement ID, its meaning, design, implementation approach, tests, AUTOSAR source, or relationship to other project requirements.
---

# Teach an AdaptivePi requirement

## Gather the actual context

1. Find the requested ID in `docs/requirements/`. Read the complete requirement entry and `docs/requirements/README.md`.
2. Inspect related requirements, ADRs, existing code, and tests only where they affect the explanation.
3. Use relevant decisions from the current conversation. Treat repository files as authoritative if remembered discussion differs from them.
4. If the ID does not exist, say so and ask for the correct ID or point to the closest matching requirement. Do not invent requirement text.
5. If precise AUTOSAR behavior matters beyond the repository's summary, check the official document linked by the requirement. Distinguish verified specification behavior from your design suggestions.

## Teach it

Use clear language suitable for a developer moving from Classic AUTOSAR and embedded C into modern C++ and Adaptive AUTOSAR concepts. Define unfamiliar C++ terms when they first matter. Prefer a small, concrete example to a long abstract explanation.

Cover, in this order:

1. **What it requires:** Restate the observable behavior in plain English, including important edge cases.
2. **Why it exists:** Connect it to the component and, where useful, a familiar embedded-software concept.
3. **How to implement it:** Identify the likely types, interfaces, ownership or state rules, and an incremental implementation sequence. Explain the reasoning behind meaningful design choices. Label proposed choices that the requirement does not prescribe.
4. **Example:** Give an example of how it should be used in the software and why it should be implemented as suggested.
5. **How to verify it:** Translate the listed verification method into concrete test cases. Include compile-time checks, death tests, or failure cases when the requirement calls for them.
6. **Scope and dependencies:** Call out stated deviations, related requirement IDs, and anything deferred to a later release.

Use short C++ snippets or pseudocode when they make a concept easier to understand. Do not present a sketch as finished, compiling project code. If the user requests a brief answer, prioritize the specific question over this full structure.

## Teach and review all supported builds

- Read `docs/adr/0006-exception-build-policy.md` and inspect current presets, target settings, and CI jobs. Implementation guidance shall cover every supported configuration affected by the requirement, including exceptions disabled and enabled.
- Provide the common implementation and all necessary conditional branches, declarations, includes, and registration code. Explain exactly where guards belong; keep ordinary Result access available in both modes. Do not present an exception-enabled-only solution as complete.
- Explain configuration-specific behavior and verification, including compile-time API availability, runtime success/failure paths, and target build constraints where relevant.
- When reviewing an implementation, inspect all affected build configurations, not only the active IDE configuration. Check guard placement, header self-containment, consistent compiler flags and definitions, and tests for both exception modes.
- Local execution uses the exception-disabled configuration. Exception-enabled tests run in CI only unless the user explicitly requests otherwise. Distinguish source review from executed checks and report missing CI evidence as pending.
- Continue guiding production changes rather than editing production code; coverage of all builds does not change this boundary.

## Project decisions to retain

- Release 3 implements project-owned, clean-room subsets of `ara::core` and `ara::log`; it does not claim full AUTOSAR API compatibility.
- `ErrorDomain` supports stable unique identity and compile-time use. `ErrorCode` can represent additional project-owned domains without changes to `ErrorCode` or `Result`.
- `Result<T, E>` and `Result<void, E>` have exactly one active alternative. Recoverable public failures use `Result`; invalid `Value()` or `Error()` access terminates. `ValueOrThrow()` provides the specified exception conversion only when exceptions are enabled and is unavailable when they are disabled.
- Release 3 logging uses a console sink, complete records across threads, controllable metadata providers in tests, and silent discard of internal logging failures.
- Modelled messages and compile-time trace routing are authored manually in C++; ARXML generation, DLT transport, and external trace-tool integration are outside Release 3.
- Later releases introduce manifests and Execution Management, communication between processes, diagnostics, and hardware deployment. Do not pull those features into a Release 3 explanation as current requirements.

## Interaction

Teach the reasoning and implementation path when the user asks for an explanation. When the user asks to implement or fix production code, guide the user rather than changing it. LLMs and AI agents are allowed to write and modify unit tests directly; for that work, follow `.agents/skills/write-unit-tests/SKILL.md`. Never silently change an approved requirement to accommodate an implementation idea.