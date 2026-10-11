# Release 4 — verification and documentation requirements

These requirements collect release evidence after manifest, controller,
supervision, demonstration and applicable logging contracts have been implemented. #41 proof work
follows the #22 functional verification work, matching the milestone dependencies.

## Requirement order

Read entries in numeric order. Every dependency is an earlier entry in this
file or an earlier component stage in [the Release 4 overview](release-4-overview.md).
Each planned check verifies only its own observable contract and earlier prerequisites;
later integration evidence does not block completion of an earlier requirement.

## AP-R4-VER-001 — Requirement verification inventory

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-MAN-015`; `AP-R4-EXEC-021`; `AP-R4-LOG-022`
- Owning issue: [#22](https://github.com/Romanr92/adaptive-pi/issues/22)
- Requirement: Release 4 shall maintain a requirement-to-implementation/check inventory covering every MAN, EXEC and LOG entry, including documented conditional applicability, exact test/check names, exception configuration and observed result without treating test declaration existence as execution success.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Documentation review against baseline and registered tests; inventory shall show missing/pending evidence explicitly.
- Unit Tests:
- Deviation: Release evidence is project-specific; Draft entries and empty Unit Tests fields do not claim implementation.

## AP-R4-VER-002 — Host and CI verification

- Status: Draft
- Dependencies: `AP-R4-VER-001`
- Owning issue: [#22](https://github.com/Romanr92/adaptive-pi/issues/22)
- Requirement: All Release 4 common host tests and compile-time checks shall pass locally with exceptions disabled and in PR CI with exceptions disabled and enabled; guarded exception-conversion paths shall execute in enabled CI, alongside existing formatting/static-analysis and prior-release checks.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Record tested SHA, configure/build/test commands, counts, skips and PR job links; local enabled execution follows the repository unit-test policy.
- Unit Tests:
- Deviation: No local enabled-build test execution is implied. Backend tests use fakes and do not require host systemd.

## AP-R4-VER-003 — ARM64 and QEMU verification

- Status: Draft
- Dependencies: `AP-R4-VER-001`; `AP-R4-VER-002`
- Owning issue: [#22](https://github.com/Romanr92/adaptive-pi/issues/22)
- Requirement: Release evidence shall include a successful ARM64 SDK build, AArch64 ELF checks and deployment/execution of EXEC-021 with startup, intentional crash, restart and controlled-stop observations, plus LOG-022 evidence for the selected Linux logging bindings. Target probes shall verify actual argument-zero/argument passing, initial empty signal mask on initial start/restart, SIGTERM graceful stop and forced cleanup of an unresponsive test execution; lifecycle evidence shall distinguish requests from observed outcomes and retain original event times.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Record SDK/image versions, tested SHA, ELF headers, deployed package paths, exit results and target output; retain a repeatable smoke procedure.
- Unit Tests:
- Deviation: QEMU verifies ARM64 Linux integration, not Raspberry Pi peripherals or realtime latency.

## AP-R4-VER-004 — API and release documentation

- Status: Draft
- Dependencies: `AP-R4-VER-001`; `AP-R4-VER-002`; `AP-R4-VER-003`
- Owning issue: [#22](https://github.com/Romanr92/adaptive-pi/issues/22)
- Requirement: Documentation shall describe manifest/package contracts, supported controller/manager interfaces, state graph, deadlines, restart budget, failure behavior, logging bindings/configuration, rotation/buffer bounds, flush/loss semantics, deferred logging deployment policy, clean-room sources and exclusions; component design decisions shall use component-local IDs with matching code comments, and README shall record the verified functional scope with any bounded-proof evidence still pending. Documentation shall cover executable argument zero, non-reporting signal handling, ordinary/forced termination, unresolved timeout cleanup and start gating, milestone/clock semantics, and the local-time notification's compatibility/filtering deviations.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Documentation/link review against actual APIs, numbered decisions, requirement inventory and recorded results.
- Unit Tests:
- Deviation: No AUTOSAR conformance claim, proprietary code, full State Management, PHM, diagnostics or machine lifecycle implementation.

## AP-R4-VER-005 — Release 4 bounded-proof coverage

- Status: Draft
- Dependencies: `AP-R4-VER-001`; `AP-R4-VER-002`; `AP-R4-VER-003`; `AP-R4-VER-004`
- Owning issue: [#41](https://github.com/Romanr92/adaptive-pi/issues/41)
- Requirement: After the #17–#22 work is verified, named ESBMC harnesses shall check pure manifest validation, permitted lifecycle transitions, restart-budget decision safety and feasible configuration/state invariants; pure unresolved-execution start gating, deadline-boundary decisions, logging configuration, rotation-boundary and queue-bound/order logic shall be assessed for feasible additional harnesses, with adopted checks and exclusions recorded; excluded available Release 4 logic shall have a documented reason.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Bounded proofs against MAN-012 and EXEC-009–011 plus other feasible pure code; each harness shall document modeled inputs, assumptions, bounds and the property checked.
- Unit Tests:
- Deviation: This follows #41 after #22; no proof of JSON5 parser libraries, filesystem races, systemd, Linux, network or third-party behavior. Core-only proofs do not satisfy this requirement.

## AP-R4-VER-006 — Reproducible proof results

- Status: Draft
- Dependencies: `AP-R4-VER-005`
- Owning issue: [#41](https://github.com/Romanr92/adaptive-pi/issues/41)
- Requirement: Each supported Release 4 proof shall have a reproducible local/CI command and recorded result tied to the harness/code revision; failure, timeout, unsupported execution or insufficient unwinding shall be reported without claiming a passing proof.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Run registered harnesses through the existing ESBMC tooling; record solver/options/bounds and results, justify exclusions, and verify a deliberately failing property is reported as failure.
- Unit Tests:
- Deviation: Reuse existing installation/gate infrastructure; bounded proof results supplement tests and QEMU evidence and do not establish full-system correctness.

## AP-R4-VER-007 — Final milestone closure

- Status: Draft
- Dependencies: `AP-R4-VER-004`; `AP-R4-VER-006`
- Owning issue: [#22](https://github.com/Romanr92/adaptive-pi/issues/22), with the proof evidence from [#41](https://github.com/Romanr92/adaptive-pi/issues/41).
- Requirement: README shall mark Release 4 complete only after the functional evidence (including applicable LOG contracts and recorded conditional outcomes) and bounded-proof results above are recorded, with every requirement accounted for and all missing, failed or deliberately excluded checks explicitly resolved or accepted before milestone closure.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi release decision.
- Verification: Final documentation review of the requirement inventory, host/CI, target scenario and proof results against the milestone acceptance criteria.
- Unit Tests:
- Deviation: Functional documentation can be finished under #22 before #41; final milestone closure follows both and does not make an earlier functional requirement depend on later proof implementation.

