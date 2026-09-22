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
4. **Example:** Give an example how it should be used in the software and why it should be implemented as suggested.
5. **How to verify it:** Translate the listed verification method into concrete test cases. Include compile-time checks, death tests, or failure cases when the requirement calls for them.
6. **Scope and dependencies:** Call out stated deviations, related requirement IDs, and anything deferred to a later release.

Use short C++ snippets or pseudocode when they make a concept easier to understand. Do not present a sketch as finished, compiling project code. If the user requests a brief answer, prioritize the specific question over this full structure.

## Project decisions to retain

- Release 3 implements project-owned, clean-room subsets of `ara::core` and `ara::log`; it does not claim full AUTOSAR API compatibility.
- `ErrorDomain` supports stable unique identity and compile-time use. `ErrorCode` can represent additional project-owned domains without changes to `ErrorCode` or `Result`.
- `Result<T, E>` and `Result<void, E>` have exactly one active alternative. Recoverable public failures use `Result`; invalid `Value()` or `Error()` access terminates. `ValueOrThrow()` provides the specified exception conversion.
- Release 3 logging uses a console sink, complete records across threads, controllable metadata providers in tests, and silent discard of internal logging failures.
- Modelled messages and compile-time trace routing are authored manually in C++; ARXML generation, DLT transport, and external trace-tool integration are outside Release 3.
- Later releases introduce manifests and Execution Management, communication between processes, diagnostics, and hardware deployment. Do not pull those features into a Release 3 explanation as current requirements.

## Interaction

Teach the reasoning and implementation path when the user asks for an explanation. Write or change repository code when the user asks to implement or fix something. Never silently change an approved requirement to accommodate an implementation idea.