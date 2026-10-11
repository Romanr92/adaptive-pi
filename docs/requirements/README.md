# AdaptivePi requirements baselines

## Purpose

This directory defines the project requirements baselines.

- Release 3: the existing core/log baseline below.
- Release 4: the [Draft baseline and dependency order](release-4-overview.md) for
  Execution Management and Manifests, with [manifest](release-4-manifests.md),
  [execution](release-4-execution-management.md),
  [logging](release-4-logging.md), and
  [verification](release-4-verification.md) requirements. The
  [AUTOSAR source completeness audit](release-4-source-audit.md) maps the
  added Draft contracts and explicit exclusions; the current baseline contains
  72 entries: MAN-001–015 are In Review and the remaining 57 are Draft.
  Supplemental stages in the overview preserve stable IDs while placing new
  prerequisites before their consumers. Review started on 2026-10-11; see the
  [review log](reviews/release-4.md).

Release 3 covers a small clean-room educational subset inspired by AUTOSAR
Adaptive Platform `ara::core` and `ara::log`.

AdaptivePi does not claim AUTOSAR conformance. The requirements are written in
project-owned wording and do not reproduce AUTOSAR specification text or
implementation code.

The accepted [exception build policy](../adr/0006-exception-build-policy.md)
requires exceptions disabled by default and an optional exception-enabled build.
AP-R3-CORE-008 defines the configuration-dependent value-access contract.
Build integration is present. Verification of the exception-enabled configuration
remains a CI obligation; see the evidence notes in the core requirement file.

## Requirement identifiers

- `AP-R3-CORE-xxx`: `ara::core`-inspired requirements
- `AP-R3-LOG-xxx`: `ara::log`-inspired requirements
- `AP-R4-MAN-xxx`: Release 4 manifest and package requirements
- `AP-R4-EXEC-xxx`: Release 4 execution-control and supervision requirements
- `AP-R4-LOG-xxx`: Release 4 logging-binding and bounded-delivery requirements
- `AP-R4-VER-xxx`: Release 4 release-evidence and bounded-proof requirements

Requirement identifiers are stable and shall not be reused for a different
requirement.

## Status

New requirements follow this lifecycle:

`Draft → In Review → Agreed → Implemented → Verified`

- `Draft`: proposed and subject to review.
- `In Review`: currently being discussed with the user.
- `Agreed`: the user explicitly agreed to this requirement's current wording.
- `Implemented`: the agreed behavior has been implemented; planned or existing
  tests alone do not establish successful verification.
- `Verified`: the planned checks have executed with accepted recorded evidence
  for the applicable implementation revision and supported configurations.

The [interactive review skill](../../.agents/skills/review-requirements/SKILL.md)
reviews one requirement at a time. Questions, discussion and agreement to an
unseen proposed change do not permit Agreed status. Changed contract wording
requires renewed review and explicit agreement. Record important decisions and
the exact accepted wording in a separate review log when review begins.
When all requirements in a requirement file have been reviewed and agreed, audit
the file's latest contracts against the baseline AUTOSAR PDFs before marking its
review complete or moving to the next file. A session pause midway through a file
saves progress without triggering this final audit. Report undeclared/misclassified deviations and unsupported
alignment/compliance claims, explain the difference, and ask the user how to
resolve it. The file's review completion remains pending until material audit findings are
resolved; changed entries require renewed agreement. This audit does not create
an AUTOSAR conformance claim or implementation-verification evidence.

Legacy `Approved` entries remain accepted baselines. Existing releases used
`Implemented` to include implementation and verification; retain those statuses,
their historical meaning and recorded evidence without automatic migration.
The new lifecycle separates implementation from verification for new work.

No release implementation shall begin until every requirement in that release
has an agreed baseline (`Agreed` or legacy `Approved`). Later Implemented/Verified
states retain acceptance for unchanged wording; pending Draft/In Review revisions
do not satisfy this gate. Release 4 remains Draft; creating this review workflow
does not agree to its requirements or authorize production implementation.

## Traceability convention

Each requirement shall contain:

1. a stable AdaptivePi requirement ID;
2. status;
3. an atomic normative requirement;
4. AUTOSAR source trace, when applicable;
5. verification method and planned test;
6. deliberate scope deviation.

An AUTOSAR source trace shall contain:

- official AUTOSAR R23-11 document link;
- document section;
- PDF page;
- AUTOSAR requirement ID, where one exists.

Requirements that are solely AdaptivePi design decisions shall explicitly state
that they have no direct AUTOSAR requirement source.

## Reviewed AUTOSAR sources

- [Specification of Adaptive Platform Core, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- [Specification of Log and Trace, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- [Guidelines for using Adaptive Platform interfaces, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
- [Specification of Platform Types for Adaptive Platform, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_PlatformTypes.pdf)

The Platform Types specification was reviewed for scope alignment. No Release 3
requirement directly traces to it because Release 3 does not implement AUTOSAR
model types, generated artifacts, or platform type definitions.

The [Release 4 logging extension](release-4-logging.md) reviews R23-11 output
bindings, identification, filtering, availability buffering and shutdown.
Journald, rotation and the global lifecycle log are explicit project decisions;
the supplied R24-11 PDF does not change the R23-11 baseline.

Logging contract clarifications and implementation-issue allocation are recorded
in [the logging baseline](release-3-ara-log.md) and
[ADR 0007](../adr/0007-release-3-logging-contracts.md).

## Release 3 exclusions

Release 3 excludes:

- full AUTOSAR API compatibility;
- generated AUTOSAR model and configuration artifacts;
- DLT and remote log transport;
- DLT transport, persistence, runtime trace configuration, external trace-tool
  integration, and filtering configuration;
- `ara::com`, Execution Management, manifests, diagnostics, and hardware
  support;
- `Future`, `Promise`, `Optional`, and other `ara::core` utilities not listed
  in the Release 3 requirements.

## Release 3 modelled messages and tracing

Release 3 implements manually authored C++ definitions for modelled log messages
and compile-time trace routing. It does not generate message definitions,
trace-routing configuration, manifests, or trace-tool specializations from
ARXML.

The Release 3 trace artifact interface is an educational clean-room boundary.
It supports unit-test verification of logger, trace-artifact, combined, and
discard routing. It does not integrate with a production trace tool.

## Source coverage

| AUTOSAR source | Release 3 use | Requirement location |
|---|---|---|
| Adaptive Platform Core | Defines ErrorDomain, ErrorCode, Result semantics and access behaviour. | `release-3-ara-core.md` |
| Log and Trace | Defines logger contexts, levels, logger ownership, and default console behaviour. | `release-3-ara-log.md` |
| Interfaces Guidelines | Guides exception-free ErrorCode and Result usage. | `release-3-ara-core.md` |
| Platform Types | Reviewed; no applicable Release 3 implementation scope. | This README |
