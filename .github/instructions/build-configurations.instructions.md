---
applyTo: "**/*.h,**/*.hpp,**/*.cpp,**/CMakeLists.txt,**/*.cmake,CMakePresets.json,.github/workflows/**/*.yml,.github/workflows/**/*.yaml"
excludeAgent: "cloud-agent"
---

Review changes against every affected supported build configuration, including
exceptions disabled and enabled. Read docs/adr/0006-exception-build-policy.md
and inspect current CMake settings, presets, and CI jobs.

Check that:
- The default build disables exceptions and the optional build enables them.
- Compiler flags and feature definitions agree and propagate to consumers.
- Headers include their required declarations in each mode independently of
  include order; configuration is included before testing its macros.
- Exception-only APIs and throwing implementations are guarded consistently.
  Ordinary Result operations remain available in both modes.
- Tests cover common behavior in both modes, conditional API availability,
  and required exception-only behavior. Apply the unit-test skill when reviewing
  tests: .agents/skills/write-unit-tests/SKILL.md.
- Applicable host and cross-build settings retain consistent exception policies.

Review both modes even when only one was executed. Exception-enabled test
execution is CI-only by default. Distinguish source inspection from build/test
evidence, report each configuration separately, and identify unverified modes.
Do not treat a passing build in one mode as evidence that all modes pass.
