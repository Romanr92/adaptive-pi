# Release 4 AUTOSAR source completeness audit

Date: 2026-10-11. Baseline: AUTOSAR AP R23-11.

This is a scoped completeness assessment, not interactive requirement review,
user agreement, implementation verification or an AUTOSAR conformance claim.
The original audit assessed 64 Draft requirements. The subsequent additions
requested by the user brought the baseline to 72 entries, initially Draft; the
disposition table below identifies their documents. MAN-001–015 are now In Review
at the user's request; the remaining 57 entries are Draft. Authoring these proposals
did not agree to them or begin interactive review.

## Draft additions following the audit

| Finding | Added contract / affected document | Draft disposition |
|---|---|---|
| F1 | [AP-R4-MAN-015](release-4-manifests.md#ap-r4-man-015--executable-argument-zero), with MAN-006 and EXEC-004 clarified | Resolved executable path at argument zero; manifest arguments begin at one. No additional manifest field. |
| F2 | [AP-R4-EXEC-022](release-4-execution-management.md#ap-r4-exec-022--initial-signal-mask-for-non-reporting-applications) | Empty blocked-signal set at application startup, verified by a target probe. |
| F3 | [AP-R4-EXEC-023](release-4-execution-management.md#ap-r4-exec-023--sigterm-first-controlled-termination) | Effective SIGTERM-first ordinary termination for generated and selected units; conflicting behavior rejected. |
| F4 | [AP-R4-EXEC-024](release-4-execution-management.md#ap-r4-exec-024--bounded-force-termination-boundary) and [EXEC-025](release-4-execution-management.md#ap-r4-exec-025--cleanup-after-a-lifecycle-deadline-expires) | Private bounded force-termination/cancellation and manager cleanup; unresolved process absence blocks another start. Function Group reactions are contextual. |
| F5 | [AP-R4-EXEC-026](release-4-execution-management.md#ap-r4-exec-026--distinct-request-and-observed-lifecycle-milestones) and [EXEC-027](release-4-execution-management.md#ap-r4-exec-027--original-lifecycle-event-occurrence-time) | Distinct request/observed milestones and original monotonic event times; no invented process creation, kRunning or exact exit-time claim. |
| F6 | [AP-R4-LOG-023](release-4-logging.md#ap-r4-log-023--local-time-base-notification) | Configured R4 mode attempts a local-time notification on successful registration; console compatibility suppresses it. Registration/filtering/compatibility departures are explicit Draft proposals. |

The overview, logging lifecycle/evidence entries and release verification plan
now include these contracts. Existing IDs and the later review position
AP-R4-MAN-001 are preserved. No source finding has been marked user-agreed.

Before drafting, five relevant official PDFs were downloaded again and their
title pages/entries rechecked: RS/TPS Manifest, RS/SWS Execution Management and
SWS Log and Trace. The latter initially timed out and succeeded on retry. The
same local certificate-chain failure required the disclosed TLS-validation
bypass; document/final-URL checks do not remove that assurance limitation.
Task-specific PDFs and extracted text were removed after drafting.

## Evidence and scope

The original audit compared MAN-001–014, EXEC-001–021, LOG-001–022 and VER-001–007 with
README, the release overview, existing Release 3 logging contracts, ADRs
0003/0004/0006/0007, and GitHub issue scope. Release 4 milestone membership
was checked in this conversation: #17–#22, #41 and #71–#73. Later consumers
include Release 5 #23–#28 and Release 6 #29–#34, particularly #31.
Image minimization #72 and hardware experiment #71 remain issue-level work.

Downloaded eleven official PDFs into task-specific temporary storage. Checked
PDF signatures, final official URLs and each title page's document name,
published status, document ID and R23-11 edition. Extracted page-separated text.
The local TLS certificate chain failed even with the system CA store; downloads
used a certificate-validation bypass for these public documents, with final URL
and document-content checks. This limits transport-authentication assurance.
The first text extractor crashed; an alternative extractor completed all files.
Temporary PDFs and extracted text were removed after this audit.

Inspected functional sections relevant to Release 4 and adjacent applicability
boundaries, rather than all requirements in the full Adaptive Platform:

| Official document | ID / PDF pages | Inspected scope |
|---|---|---|
| [Requirements on Manifest Specification](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf) | 712 / 34 | Execution deployment/startup, logging configuration, neighboring deployment capabilities (§3, especially pp. 14, 18–19). |
| [Specification of Manifest](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf) | 713 / 1086 | Process/startup model (§8, pp. 385–393); logging deployment (§10.6, pp. 482–486). |
| [Requirements on Execution Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf) | 720 / 32 | Functional requirements, pp. 12–20. |
| [Specification of Execution Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf) | 721 / 197 | Application/process distinctions, manifest, errors, lifecycle, arguments/environment (§7.2–7.4, pp. 30–51); timeout reactions (§7.5.5, pp. 70–72); scheduling/resources/working folder (§7.7.3, pp. 102–105). |
| [Specification of Log and Trace](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf) | 853 / 93 | Lifecycle, identity/configuration, existing message API, timestamps, buffering, trace and bindings (§7, pp. 17–45). |
| [Requirements of State Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_StateManagement.pdf) | 909 / 21 | Coordination and neighboring functionality (§3.1–3.2, pp. 8–10). |
| [Specification of State Management](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_StateManagement.pdf) | 908 / 105 | Configuration/recovery boundaries (§7.10–7.11.1, pp. 42–43). |
| [Requirements on Operating System Interface](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_OperatingSystemInterface.pdf) | 718 / 20 | POSIX, startup, resources and isolation (§4.2, pp. 9–12). |
| [Specification of Operating System Interface](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_OperatingSystemInterface.pdf) | 719 / 35 | Process support/isolation, scheduling and resource boundary (§7.1–7.2, pp. 14–18). |
| [Explanation of Adaptive Platform Design](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_PlatformDesign.pdf) | 706 / 93 | Library/service implementation and manifest methodology (§4.2.2–4.3, pp. 26–27); explanatory authority only. |
| [Guidelines for using Adaptive Platform interfaces](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf) | 929 / 45 | ErrorCode/Result and optional exception conversion (§3.1, pp. 8–10); explanatory authority only. |

AUTOSAR source maturity is independent of AdaptivePi Draft status. Items marked
DRAFT below retain that source maturity. Obsolete items are not obligations.
The previously supplied R24-11 logging PDF does not replace this baseline.

## Findings from the original 64-entry baseline

The coverage gaps described here refer to the baseline before the additions.
Their subsequent Draft dispositions are recorded above; proposed wording and
deviations still await later user review.

### F1 — Argument zero is cited but has no contract

- Source: SWS_ExecutionManagement §7.4.4.3, p. 50,
  SWS_EM_01072 and SWS_EM_01078. The executable name occupies argument zero;
  configured process arguments begin at argument one in configuration order.
- Current coverage: MAN-006 and EXEC-004/005 preserve ordered launch arguments
  and cite these sources, but never specify `argv[0]` or the starting index.
- Recommended disposition: strengthen the launch contract to define argument
  zero separately from manifest arguments. Specify how the project represents
  the executable name and declare any difference from the source interpretation.
- Verification: fake launch-value inspection plus a small target argument-capture
  check, including an empty argument list and arguments containing empty strings.
- Later use: Release 5 provider/client and Release 6 diagnostic-manager packages
  should reuse the same launch semantics without changing their application APIs.

### F2 — Initial signal mask for non-reporting processes is unaccounted for

- Source: SWS_ExecutionManagement §7.4.4.2, pp. 49–50,
  SWS_EM_02579 (DRAFT). Non-reporting processes start with an empty signal mask
  and supply their own signal handling. SWS_EM_02578 (DRAFT) defines a different
  mask for reporting processes using ExecutionClient.
- Current coverage: EXEC-014 intentionally uses backend activity instead of
  ExecutionClient reporting, and EXEC-020 requires graceful termination, but
  no requirement or explicit exclusion covers the initial signal mask.
- Recommended disposition: add a small non-reporting launch-condition contract
  or explicitly document the omitted behavior. Do not import the reporting-process
  mask or the ExecutionClient API into this release.
- Verification: target startup probe checks its actual blocked-signal set;
  host doubles check the adapter's intended launch conditions. Merely assuming
  that systemd supplies a suitable mask is not verification evidence.
- Later use: the same condition affects future managed services and stop behavior.

### F3 — Graceful stop does not constrain the selected unit's termination signal

- Source: SWS_ExecutionManagement §7.4.1.2, p. 39, SWS_EM_01055:
  termination starts with SIGTERM. The termination-handler API
  SWS_EM_02577 (DRAFT) is separately outside the current API subset.
- Current coverage: EXEC-007 cites SWS_EM_01055 but only requests controlled
  termination; EXEC-004 does not enumerate the unit termination settings that
  must agree with the configuration. EXEC-020 requires a clean service exit.
- Recommended disposition: define the observable SIGTERM-first contract for
  generated and selected units, while retaining systemd as the signal-delivery
  implementation. Reject incompatible selected units or explicitly declare the
  alternative signal behavior as a deviation.
- Verification: inspect unit preparation through doubles and record the service's
  graceful-stop outcome in QEMU. Test a conflicting selected unit.

### F4 — Timeout reactions need a scoped cleanup contract

- Source: SWS_ExecutionManagement §7.5.5, p. 70, SWS_EM_01065,
  SWS_EM_02255 and SWS_EM_02258; pp. 71–72, SWS_EM_02253,
  SWS_EM_02260, SWS_EM_02310 and SWS_EM_02312. These contracts operate
  during Function Group state transitions, including forced termination after
  a termination timeout and cleanup/retry handling after startup timeout.
- Current coverage: EXEC-014/015 report failed and timeout; EXEC-007 delegates
  any escalation to systemd without requiring its configuration. EXEC-017 does
  not define cleanup after a timed-out restart. A still-live or later-active
  process is therefore not given a complete management outcome.
- Applicability: Function Groups are excluded, and AUTOSAR non-reporting process
  startup does not wait for application readiness. The project's backend deadline
  is not an identical AUTOSAR startup timeout. The full source algorithm is not
  automatically a Release 4 obligation.
- Recommended disposition: define the minimal project cleanup and escalation
  behavior for its existing deadlines, including residual/late-active processes
  and prohibition of another start until the previous execution is resolved.
  Trace the source as contextual where its transition preconditions do not apply.
  Keep the explicitly documented no-initial-start-retry choice unless changed.
- Verification: fake clock/controller cases for expiry, late activity, cleanup
  success/failure and no duplicate start; QEMU stop-escalation evidence if adopted.
- Later use: Release 5 reconnect and Release 6 crash history need an accurate
  distinction between a failed manager operation and a terminated process.

### F5 — Lifecycle tracing has coverage, but its source semantics are incomplete

- Source: RS_ExecutionManagement §4.2.2, p. 18, RS_EM_00152 (DRAFT);
  SWS_ExecutionManagement §7.4.3, p. 43, SWS_EM_02573–02576 (all DRAFT).
  The source distinguishes process creation, received kRunning, termination
  request and actual termination for timing analysis.
- Current coverage: EXEC-018 and LOG-021 provide useful transition/lifecycle
  records, but EXEC-018 labels its entire behavior project-only and has no event
  occurrence timestamp. A manager's start request is not proof of process creation;
  its running observation is not receipt of kRunning.
- Recommended disposition: improve source correspondence and define which
  request/observed-process milestones are retained. Keep DLT and kRunning
  reporting excluded. Consider an occurrence timestamp in the retained event
  model so delayed logging preserves event time; this timestamp is an AdaptivePi
  foundation decision, not a field directly mandated by the cited trace entries.
- Verification: delayed event rendering with a fake clock/controller, distinguishing
  request and confirmation. Release 6 #31 is the concrete later history consumer;
  transport and persistent diagnostics remain in Release 6.

### F6 — Local time-base notification is neither adopted nor explicitly excluded

- Source: SWS_LogAndTrace §7.5, p. 36, SWS_LOG_00091 (DRAFT): logger
  creation reports whether its time base is local or globally synchronized.
- Current coverage: LOG-002 explicitly excludes synchronized time service and
  retains local UTC timestamps, but does not account for this notification.
- Disposition needing a choice: adopt a small local-time-base notification contract,
  or explicitly exclude it as part of the logging subset. Excluding synchronized
  time does not by itself account for the local-time notification. Adoption would
  affect existing console output expectations and needs an explicit compatibility
  decision against R3/LOG-005 before editing contracts.
- Verification if adopted: captured creation notification and suppression/failure
  behavior; no Time Synchronization functional cluster is necessary.

## Source behaviors already accounted for

| Source behavior | Existing treatment / audit conclusion |
|---|---|
| Executable/deployment separation: RS_MANI_00006 (DRAFT), TPS_MANI_01011 (DRAFT); process arguments/environment: SWS_EM_01012/01078/02246 | MAN-002/003/006/011–014 and EXEC-004/005 cover the focused package/model/launch subset; F1 isolates the missing argument-zero detail. |
| Empty environment values: TPS_MANI_01209 (DRAFT), SWS_EM_02249; machine environment/precedence: SWS_EM_02247/02248 | MAN-006 permits explicit empty strings and excludes Machine Manifest precedence. Missing ARXML values are represented differently by the JSON5 manifest schema; no Machine Manifest is required. Clarifying precedence over an inherited systemd environment would be a project contract refinement. |
| Startup/termination timeout configuration: TPS_MANI_01277/01278 (DRAFT) | EXEC-014/015 include configurable deadlines outside the manifest. This is an intentional configuration-source choice; F4 concerns reactions, not absence of all timeout support. |
| Startup retry metadata: Process.numberOfRestartAttempts, TPS §8.2 p. 386; SWS_EM_02260 | MAN-007 and EXEC-011/013 explicitly distinguish the crash-restart budget and exclude automatic retry of unsuccessful initial startup. This is a documented difference, not a newly missed identical crash-policy requirement. |
| Default non-self-terminating process: SWS_EM_01314; self-termination support: RS_EM_00011 | The long-running demonstrator subset in EXEC-016 treats self-exit, even zero, as unexpected. Support for legitimately self-terminating jobs is outside that subset; no one-shot application requirement is justified by current issues. |
| Reporting/non-reporting distinction: SWS_EM_01004/01402/01403 | EXEC-014 and the overview explicitly exclude ExecutionClient and business readiness. Backend active status is a project observation, not an application execution-state report. |
| Initial buffering and oldest eviction: SWS_LOG_00001 | LOG-015/016 cover retained delivery and bounds; zero-capacity disablement is a declared deviation. LOG-015 already distinguishes temporary unavailability from permanent failure, qualifying the earlier conversation's possible conflict finding. |
| Shutdown queue drain: SWS_LOG_00123 (DRAFT) | LOG-014/019 cover flush/drain, explicitly qualify delivery by a finite budget and require transports with bounded teardown. This is a declared difference, not an omitted shutdown contract. |
| Framework-owned loggers: SWS_LOG_00005; no-throw record failure: SWS_LOG_00002 (DRAFT) | R3-LOG-003/008 and R4-LOG-004/018 cover the subset. The recoverable factory remains an explicitly accepted project deviation. |
| Runtime thresholds and manifest defaults: SWS_LOG_00253/00254; dynamic mode selection: SWS §7.2.3 | LOG-003/006 explicitly defer/exclude these. Reopening immutable R3 thresholds requires a separate decision; this audit does not silently add SetThreshold. |
| Logging identity/configuration: RS_MANI_00037 (DRAFT), SWS_LOG §7.2, TPS_MANI_03162/03274 (DRAFT) | LOG-002/006/020 provide identity and internal configuration and explicitly defer manifest configuration. Application description is optional in SWS §7.2.1.1; it does not automatically require a new field. Context description already exists in R3-LOG-002. |
| Console newline and file output: SWS_LOG_00227, TPS_MANI_02386/02387 (DRAFT) | LOG-005/010 cover these. File rendering is implementation-defined; rotation, journals and persistent deployment defaults are project choices. |
| Console argument spacing/bool rendering: SWS_LOG_00228/00230 | R3 behavior differs (concatenation, true/false); LOG-005 explicitly preserves those rendering differences. Do not relabel them implementation freedom or change approved R3 behavior silently. |
| Modelled messages, type checks and compile-time trace routing: SWS_LOG_00240/00241 and SWS_LOG_20001–20004 (DRAFT) | Existing R3-LOG-010/011 and R4-LOG-003 retain the subset. Generated model-bound headers, static ARTI sink selection and DLT non-verbose encoding remain excluded. |
| OS process support/isolation: SWS_OSI_01008/01009/01010 | Existing Linux/process architecture supplies the foundation; target evidence, not new kernel implementation or ESBMC proof of the OS, is appropriate. |

## Explicit scope dispositions in the supplemented baseline

- Child-process permissions: RS_EM_00009 and SWS_EM_02558/02559 (DRAFT),
  SWS_ExecutionManagement p. 35; SWS_OSI_01014, p. 15. These are not the obsolete
  SWS_EM_01030. Access control is excluded by #17; the overview now explicitly
  accounts for omission of the default-deny/configurable child-creation behavior.
  Full enforcement is not automatically justified by the single-service demo.
- Ordinary launch scheduling policy/priority: SWS_EM_01014/01015/01041–01043,
  §7.7.3.3 p. 102; TPS_MANI_01061/01328/01188 (DRAFT), §8.3.2 p. 393.
  No schema fields or contracts adopt these. Excluding deterministic execution
  does not alone exclude ordinary initial-thread scheduling configuration. The
  overview now explicitly omits manifest-controlled policy/priority configuration,
  recording Linux/systemd deployment settings as target conditions. No fixed
  scheduling guarantee or added scheduling schema is claimed.
- Process memory limits, pre-mapping and CPU affinity: SWS_EM_02108/02109,
  resource requirements and OSI resource groups. Resource-group configuration
  and deterministic behavior are excluded, but the remaining launch-resource
  configuration omissions are now explicit in the overview rather than implied.
- Process-state milestones: SWS_EM_01002/01003/01404/01006 describe actual
  process conditions. EXEC-009/010 are explicitly a project manager abstraction,
  including request-time states. Their references provide context, not evidence
  of equivalence to AUTOSAR Process States.
- Full machine/Function Group management, execution dependencies, PHM, IAM,
  trusted-platform checks, ARXML, DLT/network diagnostics and production safety
  are documented exclusions. Sources inspected do not justify importing these
  neighboring features into Release 4.

## Traceability corrections applied and interpretation cautions

- Many TPS_MANIFEST source entries and RS_MANI_00006 are DRAFT in AUTOSAR;
  that maturity is now explicit in the affected MAN/EXEC source traces, as already
  done for several logging entries. Published document status is not entry maturity.
- MAN-003/011 cite TPS_MANI_01012/01013. These describe state-dependent
  startup modelling, not the exact JSON5 fields or owned C++ storage. They are
  now marked contextual at p. 389; applicable argument/environment entries remain.
- EXEC-001 now also cites SWS_EM_02548 (§7.3.1 p. 37) for creating/converting
  error information; SWS_EM_02547 alone concerns obtaining it. No new error-domain
  requirement was added merely to improve this trace.
- LOG-014 uses the queued-message shutdown source as context for a file flush;
  it is not a direct source requirement for a particular storage flush operation;
  LOG-014 now states that distinction.
- SWS_LOG_00001 p. 17 names LogAndTraceInstantiation.queueSize, while the
  R23-11 TPS table 10.57 p. 484 puts queueSize on DltLogSink (table 10.56 has
  no queueSize). This same-edition source inconsistency is now recorded in the
  logging source notes; the observable queue/oldest-eviction contract remains clear.

## Remaining later-review decisions

The added requirements are proposals ready for later review. Review should
challenge the resolved-path interpretation of executable name, force-termination
and confirmation budgets, unresolved-execution behavior, event timing-episode
representation, and LOG-023's registration/console/filtering compatibility policy.
Concrete timeout values, internal API spellings, time representations and adapter
transport remain component design choices where the contracts leave them open.
Final sink defaults/retention/persistent storage policy remain intentionally deferred.
Scheduling/resources/access control remain explicit scope exclusions; adding them
would require a scope decision rather than silently importing more AUTOSAR features.

This audit identifies missing coverage and unclear disposition within the inspected
Release 4 subset. It does not establish exhaustive AUTOSAR coverage. No approval
review has started, no test evidence has been created, and no production code has
been changed.
