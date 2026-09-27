---
applyTo: "**/tests/**/*.cpp"
excludeAgent: "cloud-agent"
---

When reviewing unit tests, read and apply
.agents/skills/write-unit-tests/SKILL.md.

Report violations in added or modified tests. Identify the violated
rule and explain the concrete correction.

Check especially:
- Requirement traceability and coverage of the approved behavior.
- Parameterization when cases share setup, execution, and assertions;
  preserve the skill's exceptions where standalone tests are clearer.
- Separate parameter structures, printers, classes, name generators,
  and instantiations for each TEST_P.
- Requirement blocks and exactly 89-character delimiter comments.
- Lifecycle fixtures confined to the documented Fixtures section.
- C-style documentation immediately above each test and explicit
  Arrange, Act, Expect sections inside every test body.
- Updated requirement Unit Tests inventories and verification markers.

Distinguish missing test coverage from formatting violations.
Do not claim tests pass unless execution evidence is available.

Review coverage for both exception-disabled and exception-enabled builds, following
the skill's configuration rules. Common tests must remain active in both modes;
exception-only support and tests must be guarded. Check compile-time availability
of conditional APIs and preserve successful-path coverage in the disabled build.
Exception-enabled tests run in CI, not locally by default. Report the verification
status of each mode separately; a passing disabled build does not verify both.
