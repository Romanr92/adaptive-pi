# Release 4 requirements baseline — Execution Management and Manifests

## Status and scope

This is a **Draft** baseline for issues [#17–#22](https://github.com/Romanr92/adaptive-pi/milestone/2),
[#41](https://github.com/Romanr92/adaptive-pi/issues/41), and the logging extension
[#73](https://github.com/Romanr92/adaptive-pi/issues/73), reviewed against the
official AUTOSAR AP R23-11 documents on 2026-10-11. The baseline contains 72
Draft requirements: MAN-001–015, EXEC-001–027, LOG-001–023 and VER-001–007.
Creating these files does not approve them. No Release 4 production implementation may begin until every
Release 4 requirement is Agreed (or legacy Approved) under AGENTS.md.

Release 4 defines one packaged application, a manually authored manifest, a
systemd controller boundary, a small manager state machine, bounded automatic
restart decisions, and a supervised platform-test-service demonstration. It
also adds [logging bindings and bounded delivery](release-4-logging.md) under #73,
and uses existing Release 3 core/log interfaces and exception policy. Requirements
are project-owned paraphrases; no proprietary implementation or specification
text is reproduced and no AUTOSAR conformance is claimed.

The [source completeness audit](release-4-source-audit.md) maps findings F1–F6
to the added Draft contracts and records explicit scope exclusions. The added
entries have source/deviation notes and planned checks, with no approval or
implementation evidence. Authoring can continue before the user starts review.

## Next review session

Review has not started. When the user chooses to begin later, start
with **AP-R4-MAN-001** in [Manifest and package](release-4-manifests.md). Follow
the [interactive review skill](../../.agents/skills/review-requirements/SKILL.md)
and this explicit dependency order, taking each range in ascending numeric order.
Supplemental stages preserve existing IDs:

1. **AP-R4-MAN-001–009, then MAN-015:** manifest/package and argument-zero specification (#17).
2. **AP-R4-MAN-010–014:** manifest loading and validation (#18).
3. **AP-R4-EXEC-001–008:** controller boundary and backend operations (#19).
4. **AP-R4-EXEC-022–024:** initial signal mask, SIGTERM-first stop and force-termination boundary (#19).
5. **AP-R4-EXEC-009–017:** lifecycle states and restart management (#20).
6. **AP-R4-EXEC-025:** lifecycle timeout cleanup (#20).
7. **AP-R4-EXEC-018, then EXEC-026–027:** bounded history, observed milestones and original event timing (#20).
8. **AP-R4-EXEC-019–021:** target demonstration and deployment (#21).
9. **AP-R4-LOG-001–021:** logging bindings, bounded delivery and lifecycle log (#73).
10. **AP-R4-LOG-023, then LOG-022:** local time-base notification, then logging evidence (#73).
11. **AP-R4-VER-001–004:** functional verification and documentation (#22).
12. **AP-R4-VER-005–006:** bounded proofs (#41).
13. **AP-R4-VER-007:** final release closure (#22/#41).

For each requirement:

- Explain its intent and distinguish AUTOSAR source behavior from AdaptivePi choices.
- Check its dependencies and planned verification.
- Discuss questions and whether additional or revised deviations are needed.
- Record agreed edits and unresolved questions before moving to the next requirement.

At the end of each session, update this note with the next requirement to review
and any open questions or pending file audit. Once all requirements in a file are
reviewed and agreed, perform the review skill's final audit against the baseline
AUTOSAR PDFs before marking that file complete or moving to the next file. Report
any hidden deviation or unsupported alignment claim and ask the user for the next
steps. Pausing a session midway through a file saves progress without triggering
this final audit.
Explicit agreement to each requirement in the full
baseline precedes Release 4 implementation; this walkthrough does not agree to
requirements automatically.

## Implementation order

| Order | Requirement file / IDs | Work and owning issues |
|---|---|---|
| 1 | [Manifest and package](release-4-manifests.md), MAN-001–009, then MAN-015 | Specify syntax, package, fields, value rules, restart configuration, unit names, examples and executable argument zero (#17); documentation only. |
| 2 | [Manifest and package](release-4-manifests.md), MAN-010–014 | Error domain, owned model, pure validation, filesystem validation, then loading/parsing pipeline (#18). |
| 3 | [Execution Management](release-4-execution-management.md), EXEC-001–008 | Error/observation model, fake controller, safe unit preparation, backend start/stop/query (#19). |
| 4 | [Execution Management](release-4-execution-management.md), EXEC-022–024 | Non-reporting launch mask, effective ordinary termination signal and bounded force-termination/cancellation boundary (#19). |
| 5 | [Execution Management](release-4-execution-management.md), EXEC-009–017 | State model, transition relation, pure restart decision, configured manager, startup, confirmation, stop, crash detection and restart (#20). |
| 6 | [Execution Management](release-4-execution-management.md), EXEC-025 | Timeout cleanup, unresolved execution and prevention of duplicate/late starts (#20). |
| 7 | [Execution Management](release-4-execution-management.md), EXEC-018, then EXEC-026–027 | Bounded history, distinct request/observed milestones and preserved original event times (#20). |
| 8 | [Execution Management](release-4-execution-management.md), EXEC-019–021 | Target service package, launch/signal probes, fault trigger, deployment and crash/restart scenario (#21). |
| 9 | [Logging](release-4-logging.md), LOG-001–021 | Internal records/configuration, fake-sink delivery, console, journald feasibility, files/rotation, buffering, optional workers, shutdown, stream separation and global lifecycle stream (#73). |
| 10 | [Logging](release-4-logging.md), LOG-023, then LOG-022 | Local time-base notification with compatibility suppression, then logging evidence (#73). |
| 11 | [Verification](release-4-verification.md), VER-001–004 | Functional evidence, host/CI, ARM64/QEMU and API/release documentation (#22). |
| 12 | [Verification](release-4-verification.md), VER-005–006 | Bounded proofs of available Release 4 pure logic and reproducible proof results (#41). |
| 13 | [Verification](release-4-verification.md), VER-007 | Final milestone closure after functional and proof evidence (#22/#41). |

Logging entries depend on earlier LOG stages and existing core/log
contracts, with EXEC-018/026/027 added for lifecycle records and EXEC-021 for evidence.
Fake factories and sinks, storage doubles and explicit drain steps finish earlier
contracts before later bindings, workers and target integration exist.

Image minimization (#72) and the Raspberry Pi smoke experiment (#71) remain
issue-level work without new requirement IDs. Their issue ordering/gating rules
are not changed by this logging extension.

Full IDs use `AP-R4-MAN-xxx`, `AP-R4-EXEC-xxx`, `AP-R4-LOG-xxx`, and `AP-R4-VER-xxx`.
Dependencies refer only to lower-numbered IDs in a file or an explicitly earlier
component stage in this table, including supplemental stages within a file.
They include documentation contracts as well as implemented behavior. For
example, pure restart decisions can be tested before any automatic restart is
connected to a backend. Fake backend and clock inputs verify earlier behavior
without requiring the later target application. Later integration checks add
release evidence; they are not prerequisites for an earlier unit-level check.

Issue #22 is functional closure evidence; #41 explicitly follows #17–#22. Final
milestone closure includes #41 too. This separation avoids making #22 depend on
#41 while #41 depends on #22. Existing ESBMC installation/CI infrastructure can
be reused; new proofs must cover Release 4 logic rather than only core examples.

## Proposed AdaptivePi choices requiring approval

These details make the issue scope testable, but were not predetermined by
AUTOSAR or by the issue titles:

- JSON version 1, strict unknown/duplicate-key rejection, a defined package tree,
  field names, identity/unit-name grammar and path containment rules.
- Only never/on_failure restart modes with 0–100 additional starts per explicit
  start episode, no reset on successful automatic restart, no automatic retry
  of an unsuccessful initial start, and no backoff.
- A stopped manager state in addition to initializing/running/terminating/failed/
  restarting, so initial state and successful shutdown have defined endpoints.
- systemd active status as startup confirmation; it does **not** establish
  application business readiness or implement ara::exec::ExecutionClient reporting.
- Resolved executable path as argument zero, empty initial signal mask for the
  non-reporting subset, SIGTERM-first ordinary stop and a private force-termination
  operation that cancels pending activation before confirming execution absence.
- Timeout cleanup with an unresolved-execution gate, no duplicate/late starts,
  and a finite confirmation budget; failure is not proof of process termination.
- Logging destination configuration, journald priority/source metadata mapping,
  bounded rotating files, byte-bounded oldest-entry buffering, optional workers,
  finite shutdown budgets and a minimal global lifecycle stream (LOG-001–023).
  Long-term sink selection and persistent storage policy remain open.
- Local time-base notification on successful registration in configured R4 mode,
  suppressed in the console-only compatibility mode; ordinary Info filtering and
  non-fatal delivery apply. This is a Draft compatibility proposal for later review.
- Manager-controlled restart with systemd automatic restart disabled, serialized
  manager use, generation-based failure deduplication, injectable clock deadlines,
  finite in-memory event history, distinct request/observed milestones and original
  monotonic event times separate from logging submission timestamps.

These choices are explicitly Draft. Approved edits must preserve the dependency
order or update dependencies and their verification together. API spelling,
parser-library selection, systemd transport and concrete storage remain
implementation/design decisions where the requirements do not prescribe them.

## Source review and scope correspondence

The original ten supplied PDFs were extracted and checked for release identification and
relevant sections. PDF page numbers below are one-based and match document
pagination. Full official URLs, sections, pages and applicable source IDs appear
in individual requirement entries. Source entries marked DRAFT or OBSOLETE in
AUTOSAR retain their source maturity; AdaptivePi review status is separate.
Obsolete AUTOSAR entries are not used as normative implementation requirements.

| Reviewed baseline source (R23-11) | Relevant expectations | Release 4 treatment |
|---|---|---|
| [Specification of Log and Trace](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf), §7.1–7.2, pp. 17–22; §7.5–7.6, pp. 36–37; §7.8, pp. 42–45 | Identification, configuration, error containment, time-base notification, output bindings, buffering and shutdown. | LOG-001–023 retain the focused R3 API and distinguish source oldest-entry eviction from project rotation/threading/journald choices. LOG-023 declares registration/compatibility/filtering deviations. Source DRAFT maturity remains explicit. |
| [Requirements on Manifest Specification](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf), §3.1.1.2, p. 14, RS_MANI_00006/00007 | Deployment ties an executable to a process; startup configuration can depend on platform states. | MAN package/configuration scope uses the deployment concept; machine/function-group configuration is excluded. |
| [Specification of Manifest](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf), §8, pp. 385–393 | Process, startup configuration, process arguments, process-scoped environment, startup/termination timeouts and startup retry metadata. | Manually authored schema/configuration replaces the meta-model. Environment and launch values are retained; crash-budget semantics are a documented project deviation. |
| [Requirements on Execution Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf), §4.2.3–4.2.4, pp. 18–19, RS_EM_00101/00103/00150 | Lifecycle coordination, State Management interaction and execution error handling. | One-application lifecycle/error handling; no Function Groups, StateClient or system-wide error recovery. |
| [Specification of Execution Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf), §7.2.5–7.4.4.4, pp. 33–51, and §7.7.3.5, p. 105 | Execution manifests, lifecycle, termination, argument/environment passing and implementation-specific working folders. | systemd adapter and project manager; checked argument transport and explicit working directory. Full ara::exec API, deployment tooling and machine startup are excluded. |
| [Requirements of State Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_StateManagement.pdf), §3.1–3.2.1, pp. 8–9, RS_SM_00001/00005 | Platform-specific coordination of application sets and internal states. | Reviewed as a boundary: the Release 4 manager is not a State Management functional cluster. |
| [Specification of State Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_StateManagement.pdf), §7.1.2, p. 27, and §7.11.1, pp. 42–43 | Custom machine behavior and recovery coordination with EM/PHM. | A local crash-restart policy only; no SM state machines, recovery tables, PHM or machine reset. |
| [Requirements on Operating System Interface](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_OperatingSystemInterface.pdf), §4.2, pp. 9–10, RS_OSI_00100/00105 | PSE51 application interface and EM initial-process startup. | Linux/systemd already exists; no new PSE51 implementation, OS conformance claim or EM-as-PID-1 behavior. |
| [Specification of Operating System Interface](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_OperatingSystemInterface.pdf), §7.1.2, pp. 14–15, SWS_OSI_01040/01008/01009 | Initial EM, process support and isolation. | Separate Linux processes with systemd backend; systemd and kernel behavior is integration-tested, not implemented or bounded-proved by this release. |
| [Explanation of Adaptive Platform Design](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_PlatformDesign.pdf), §4.2.2–4.3, pp. 26–27 | Functional-cluster implementation choices, interaction and manifest methodology. | Architectural context for package/configuration separation and systemd boundary, not additional normative requirements. |
| [Guidelines for using Adaptive Platform interfaces](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf), §3.1–3.1.2.2, pp. 8–10 | ErrorCode/Result and explicit recovery. | Existing core types for recoverable errors in both modes; guarded exception conversion follows ADR 0006. |

The supplemental source audit inspected SWS_ExecutionManagement §7.4.3
(p. 43), §7.4.4.2–3 (pp. 49–50) and §7.5.5 (pp. 70–72), plus
RS_ExecutionManagement §4.2.2 (p. 18). EXEC-022/023 adopt the applicable
non-reporting launch mask and ordinary SIGTERM behavior. EXEC-024/025 use
Function Group timeout reactions as context for project deadlines, without
importing Function Groups or startup retries. EXEC-026 explicitly adapts the
source milestone tracing to observable systemd outcomes; EXEC-027 adds event
timing for the concrete later Release 6 #31 history consumer. DLT remains excluded.

The logging source was reviewed separately against R23-11 (93 pages); the newly
supplied local logging PDF is R24-11 (121 pages). Its edition does not replace
the baseline or its section/page references. See the logging file for conditional
journald support and intentionally deferred deployment policy.

AUTOSAR process states, application-reported execution states, machine/function-
group states and systemd unit states are distinct concepts. The proposed manager
states are a small project abstraction and must not be described as the complete
AUTOSAR state model. AUTOSAR manifest format processing also does not make this
manually authored JSON format an AUTOSAR processed manifest.

## Exclusions

Release 4 excludes ARXML parsing/generation, full API/ABI compatibility, machine
and Function Group state management, PHM, deterministic execution/scheduling,
resource-group configuration, machine reset, trusted-platform/IAM security,
package signing/installation/updates, multi-application dependency graphs,
distributed supervision, persistent diagnostics/health records, DLT and external
diagnostic integration, hardware-specific validation, and production safety claims.

The reduced launch schema also omits configurable initial-thread scheduling
policy/priority, CPU affinity, process memory budgets and pre-mapping. Ordinary
scheduling configuration is a separate omitted capability, not implied by the
deterministic-execution exclusion. Linux/systemd deployment settings remain
outside this manifest and are recorded as target conditions, with no real-time
guarantee. Sources: SWS_ExecutionManagement §7.7.3.3–4, pp. 102–104,
SWS_EM_01014/01015/01041–01043/02108/02109; TPS_ManifestSpecification
§8.3.2, p. 393, TPS_MANI_01061/01328/01188 {DRAFT}.

Child-creation permission configuration and default-deny enforcement are also
excluded under #17's access-control boundary: SWS_ExecutionManagement §7.3,
p. 35, SWS_EM_02558/02559 {DRAFT}. The obsolete SWS_EM_01030 is not used
as an obligation. The demonstration does not need managed child processes;
this omission is not a claim that Linux inherently enforces that source policy.

## Verification convention

Each entry has status, explicit dependencies, owning issue, normative behavior,
source trace, planned verification, empty Unit Tests inventory, and deviation.
Empty inventories and planned suites are not passing evidence. No test or
production implementation is created by these files. Local execution uses the
default exceptions-OFF build; enabled-mode execution is in CI under the unit-test
skill. Accepted completion evidence must identify the revision and mode tested.
