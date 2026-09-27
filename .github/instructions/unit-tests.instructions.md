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
