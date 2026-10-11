# Release 4 — logging bindings and bounded delivery requirements

This **Draft** extension belongs to [issue #73](https://github.com/Romanr92/adaptive-pi/issues/73).
It preserves the existing Release 3 logging API and adds platform-owned bindings,
bounded delivery and a minimal lifecycle log. No production implementation or
requirement approval is created by this document.

## Source edition and interpretation

Reviewed on 2026-10-11 against AUTOSAR AP **R23-11**, Document ID 853,
Specification of Log and Trace (93 PDF pages), matching the repository baseline
and issue #73. The supplied local `AUTOSAR_AP_SWS_LogAndTrace.pdf` identifies
itself as **R24-11** (121 pages); it was inspected for edition/context, but its
section/page numbers and revised configuration names are not used as R23-11
traceability. Source maturity labels such as `{DRAFT}` are retained independently
of AdaptivePi approval status.

Requirements are project-owned paraphrases. Explanatory source sections provide
conceptual traceability, not an invented normative AUTOSAR requirement ID.
Journald, rotation, byte capacities, threading, file separation and the global
lifecycle stream are AdaptivePi decisions. File output in the source is for
development/integration/prototyping; this demonstrator makes no production
persistence or AUTOSAR conformance claim.

## Dependency and verification order

Read LOG-001–022 in ascending order after the existing MAN/EXEC stages and
before VER-001–007. Every LOG dependency is a lower-numbered entry; LOG-021/022
also use already-defined execution contracts. Each earlier requirement can be
finished with its own documentation review, pure check or fake boundary; it does
not require a later concrete sink, worker, lifecycle integration or QEMU check.
For example, LOG-006 uses fake factories, LOG-007 uses fake destinations and
LOG-015/016 use explicit drain steps. LOG-022 collects later release evidence.
Existing Release 3 core/log contracts and ADR 0006 apply throughout.

## Proposed decisions and deferred policy

Oldest-record eviction follows SWS_LOG_00001; a byte bound, whole-record discard
for oversized inputs, zero-capacity disablement and finite shutdown budget are
explicit project choices requiring review. Asynchronous workers are optional;
availability buffering is a separate contract. Journald is conditional on a
recorded feasibility outcome. A rotating file binding is required.

The current experimental configuration and concrete limits must be documented,
but default sink combinations, selection scope, long-term paths/retention,
persistent partitions and future manifest configuration remain open. No new
logging fields are added to MAN-001–014. `/var/log/adaptive-pi/` is a suggested
configurable experimental root, not a permanent production location.

Image minimization (#72) and the Raspberry Pi smoke experiment (#71) remain
issue-level work and have no new requirements in this logging extension.

## AP-R4-LOG-001 — Sink-independent logging boundary

- Status: Draft
- Dependencies: Existing Release 3 core/log contracts only; no Release 4 prerequisite.
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Logging through the existing Logger, LogStream and modelled-message APIs shall remain independent of concrete console, journald, file and queue implementations; backend configuration shall remain below the application logging API.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.8.1, PDF pp. 42, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_001_ApiBoundary`: Review public headers and compile existing logging callers without backend-specific includes or new call arguments.
- Unit Tests:
- Deviation: No full AUTOSAR API/ABI compatibility or public sink-selection API is introduced. This extends the Release 3 internal sink boundary.

## AP-R4-LOG-002 — Completed record and owned metadata

- Status: Draft
- Dependencies: `AP-R4-LOG-001`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: A completed internal record shall preserve payload, severity, context ID, application ID, timestamp, PID and submitting TID as one logical snapshot; retained records shall own their data after the submitting stream and temporary inputs cease to exist.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.1–7.2.5, PDF pp. 18–21, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf); [Specification of Log and Trace, AUTOSAR AP R23-11, §7.5, PDF pp. 36, SWS_LOG_00082, SWS_LOG_00083](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_002_RecordOwnership`: Use controlled metadata providers; verify fields and lifetime after input destruction, including delayed inspection without any worker or real sink.
- Unit Tests:
- Deviation: The internal representation, Linux PID/TID and submission-time snapshot are project choices. Local UTC time follows R3; synchronized time service, DLT headers, ECU/session identity and multi-instance deployment are excluded. Application identity is supplied by platform configuration; no logging fields are added to the manifest.

## AP-R4-LOG-003 — Existing filtering and trace semantics

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-002`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: A record shall enter logger-destination delivery only when admitted by its existing context threshold; Off and invalid severities shall produce no logger-destination record, while compile-time trace routing shall retain the independent behavior defined by AP-R3-LOG-011.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.2, PDF pp. 19, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf); [Specification of Log and Trace, AUTOSAR AP R23-11, §7.7.2, PDF pp. 38–41, SWS_LOG_20001–SWS_LOG_20004](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_003_Filtering`: Check severity/threshold pairs with a capturing boundary; verify filtering precedes metadata acquisition and logger delivery while configured trace invocation remains independent.
- Unit Tests:
- Deviation: Creation-time thresholds remain the Release 3 contract. Application-wide runtime adjustment, SetThreshold, manifest-derived defaults and remote filtering are deferred; SWS_LOG_00253/00254 are not adopted by this extension.

## AP-R4-LOG-004 — Non-fatal backend failure

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-002`; `AP-R4-LOG-003`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Record preparation and delivery failures shall remain internal and shall neither throw into nor terminate the logging caller, propagate an application error, or recursively log through the failing path; a failed delivery shall discard the affected record for that destination.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.6, PDF pp. 22, SWS_LOG_00002 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_004_FailureContainment`: Inject preparation and single-sink failure statuses; verify caller continuation, no recursive writes and no partial record submission. Enabled CI additionally injects internally contained exceptions.
- Unit Tests:
- Deviation: Explicit status and non-throwing paths apply in both exception modes under ADR 0006. Recoverable platform configuration failures are separate from ordinary logging calls. This does not promise recovery from arbitrary process-wide allocation failure.

## AP-R4-LOG-005 — Console compatibility

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-002`; `AP-R4-LOG-003`; `AP-R4-LOG-004`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: With the console destination selected, existing console output shall retain the stdout format, escaping, UTC metadata, whole-record discard and complete-record threading contracts of AP-R3-LOG-004–009.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.8.2.1, PDF pp. 42, SWS_LOG_00227](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_005_ConsoleCompatibility`: Run the existing console regression cases against the console binding and verify one newline and unchanged bytes for representative stream/modelled records.
- Unit Tests:
- Deviation: The R3-only requirement that every logger uses console remains its historical baseline; R4 adds platform-selected bindings while preserving a console-only compatibility configuration. Existing project rendering deviations remain; no new argument spacing or bool representation is imposed.

## AP-R4-LOG-006 — Internal destination configuration

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-002`; `AP-R4-LOG-003`; `AP-R4-LOG-004`; `AP-R4-LOG-005`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Platform-owned logging configuration shall describe the selected destinations and their settings independently of public logging calls; invalid or unsupported settings shall fail configuration without publishing a partially configured dispatcher.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.3–7.2.3.1, PDF pp. 20–21, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_006_Configuration`: Validate empty, console-only, duplicate and unsupported destination selections with in-memory descriptors/fake factories; verify prior configuration is unchanged on failure.
- Unit Tests:
- Deviation: An empty set deliberately discards logger output. Configuration uses project-owned status/Result in both modes. Concrete storage and global/per-application/per-context selection policy remain design choices; ARXML, Machine Manifest and public dynamic mode changes are excluded. Fake factories suffice before concrete sinks exist.

## AP-R4-LOG-007 — Delivery to selected destinations

- Status: Draft
- Dependencies: `AP-R4-LOG-002`; `AP-R4-LOG-003`; `AP-R4-LOG-004`; `AP-R4-LOG-006`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Each admitted completed record shall be offered once to each selected destination and to no unselected destination; one destination failure shall not prevent offers of the same logical record to the remaining destinations.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.8.1, PDF pp. 42, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_007_SelectedDelivery`: Use only capturing/failing fake sinks to check zero/one/multiple selections, identical snapshots, failure isolation and no duplicate offers.
- Unit Tests:
- Deviation: Fan-out and failure isolation are project choices; an offer is not a guarantee of physical delivery or durable storage. Journald and rotating files are not prerequisites for this requirement.

## AP-R4-LOG-008 — Journald feasibility outcome

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-006`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: The project shall record whether a journald binding is supported in the Release 4 Yocto image, with the evaluated integration/dependency approach and an explicit reason when support is not feasible.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_008_JournalFeasibility`: Review a documented target capability/dependency probe and supported/unsupported outcome; this check does not require a completed journald sink.
- Unit Tests:
- Deviation: Issue #73 permits documented feasibility instead of journald implementation. LOG-009 is conditional on a supported outcome; unsupported status is recorded explicitly, not as passing journald delivery.

## AP-R4-LOG-009 — Journald record mapping

- Status: Draft
- Dependencies: `AP-R4-LOG-002`; `AP-R4-LOG-003`; `AP-R4-LOG-004`; `AP-R4-LOG-008`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: When LOG-008 selects journald support, the binding shall submit payload, context ID, application ID and source timestamp/PID/TID without silently replacing source identity with worker identity; severity shall map Fatal/Error/Warn/Info/Debug/Verbose to journal priorities 2/3/4/6/7/7 respectively.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_009_JournalMapping`: Use a fake journal transport to inspect every severity and identity field, special-character/empty payload handling, and submission failure without a running journal.
- Unit Tests:
- Deviation: The Linux metadata mapping and numeric priorities are proposed project choices, not AUTOSAR DLT mapping. Off produces no record. Native journal reception metadata may coexist with explicitly named source fields; final field names are a component design decision. This entry is conditional on LOG-008.

## AP-R4-LOG-010 — Configurable file destination

- Status: Draft
- Dependencies: `AP-R4-LOG-002`; `AP-R4-LOG-003`; `AP-R4-LOG-004`; `AP-R4-LOG-006`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: The file binding shall support platform-configured output paths and append complete framed records preserving LOG-002 metadata; changing the log root shall require no application logging API changes.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.3.1, PDF pp. 21, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf); [Specification of Log and Trace, AUTOSAR AP R23-11, §7.8.3, PDF pp. 44, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_010_FileAppend`: Use temporary directories and a storage double to verify create/append, record framing, metadata and alternate roots without rotation or an asynchronous worker.
- Unit Tests:
- Deviation: /var/log/adaptive-pi/ is a suggested experimental root only. File naming/encoding are documented implementation decisions. AUTOSAR describes file output for development/integration/prototyping, not production suitability; no production persistence claim is made.

## AP-R4-LOG-011 — File rotation configuration

- Status: Draft
- Dependencies: `AP-R4-LOG-006`; `AP-R4-LOG-010`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: File configuration shall specify a positive maximum byte size for each active/archive file and a finite non-negative archive count; invalid or unrepresentable limits shall be rejected before writing or changing existing files.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_011_RotationConfiguration`: Check zero, negative, fractional, unrepresentable and valid boundary limits using configuration values and a storage double; assert no storage mutation on rejection.
- Unit Tests:
- Deviation: Limits, units and rotation retention are project-owned. Archive count excludes the single active file. No permanent deployment size/count is fixed by this requirement.

## AP-R4-LOG-012 — Bounded deterministic rotation

- Status: Draft
- Dependencies: `AP-R4-LOG-010`; `AP-R4-LOG-011`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Before appending a framed record that would exceed the configured active-file byte limit, the file binding shall rotate oldest retained data out and start a new active file using a documented deterministic archive order; a record larger than the limit shall be discarded whole without rotation. Under successful storage operations the binding shall retain at most one active file and the configured archive count, each within the byte limit.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_012_RotationBoundaries`: Use storage doubles and temporary files for exact-fit/one-byte-over, zero archives, repeated rotation, oversized records and pre-existing oversized active files; define normalization of existing files before accepting a write.
- Unit Tests:
- Deviation: Rotation algorithm and archive names are project choices. Bound covers managed files, not unrelated disk usage or filesystem overhead. LOG-013 separately specifies partial-operation failures; crash-atomic rotation and multi-process shared-file writers are excluded.

## AP-R4-LOG-013 — Storage failure and recovery policy

- Status: Draft
- Dependencies: `AP-R4-LOG-004`; `AP-R4-LOG-010`; `AP-R4-LOG-011`; `AP-R4-LOG-012`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Open, append, flush, rename/remove and storage-full/unavailable failures shall obey LOG-004 and leave the binding in a documented state that prevents unbounded file creation or retry loops; subsequent calls shall follow an explicit bounded recovery-or-discard policy.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.6, PDF pp. 22, SWS_LOG_00002 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_013_StorageFailure`: Inject each operation failure and partial writes with a storage double; verify caller continuation, bounded retries/files, no recursive log and the documented subsequent-call behavior.
- Unit Tests:
- Deviation: Recovery policy must be recorded as a component design decision before implementation. OS partial writes may leave an incomplete tail; the framing/recovery documentation must explain detection. Transactional storage, filesystem repair and recovery after power loss are excluded.

## AP-R4-LOG-014 — Explicit file flush contract

- Status: Draft
- Dependencies: `AP-R4-LOG-004`; `AP-R4-LOG-010`; `AP-R4-LOG-013`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: The file binding shall provide an internal explicit flush operation that attempts to pass buffered bytes to the underlying storage and reports success/failure to platform lifecycle code without changing ordinary logging-call behavior.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.1.2, PDF pp. 17–18, SWS_LOG_00123 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_014_FileFlush`: Use a storage double for successful/failed flush and empty-buffer behavior; check exactly which storage action constitutes success without any dispatcher queue.
- Unit Tests:
- Deviation: Flushing a file buffer is distinct from LogStream::Flush and from draining a dispatcher queue. Durability across power loss, fsync policy and guaranteed persistence across reboot are not implied.

## AP-R4-LOG-015 — Bounded unavailable-destination buffering

- Status: Draft
- Dependencies: `AP-R4-LOG-002`; `AP-R4-LOG-004`; `AP-R4-LOG-006`; `AP-R4-LOG-007`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: When a selected destination temporarily cannot process records, its pending-delivery buffer shall retain owned records within configured finite record and byte capacities; on capacity overflow it shall discard oldest pending records until the new record fits. A record exceeding the entire byte capacity shall be discarded whole.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.1.1, PDF pp. 17, SWS_LOG_00001](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_015_UnavailableBuffer`: Drive a fake unavailable sink with explicit drain steps; verify capacity boundaries, oldest eviction, oversized-record discard and no growth beyond either bound without a worker.
- Unit Tests:
- Deviation: This adopts the source oldest-entry rule and adds a byte bound. A zero capacity explicitly disables buffering as a project deviation. Temporary unavailability is distinct from permanent record failure under LOG-004; no AUTOSAR daemon or core Initialize API is added. Startup buffering does not require asynchronous dispatch.

## AP-R4-LOG-016 — Buffered delivery ordering

- Status: Draft
- Dependencies: `AP-R4-LOG-007`; `AP-R4-LOG-015`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: On destination recovery, retained records shall be offered in their acceptance order before newer records for that destination; successfully delivered records shall not be replayed and another destination shall not receive duplicate records merely because this destination recovers.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.1.1, PDF pp. 17, SWS_LOG_00001](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_016_BufferRecovery`: Use explicit drain steps and independently available/unavailable fake sinks to check order, eviction gaps, repeated recovery and no replay to healthy destinations.
- Unit Tests:
- Deviation: Ordering and per-destination pending delivery are project decisions. Concurrent callers have dispatcher acceptance order, not a guaranteed wall-clock order. Backend success cannot establish exactly-once physical storage after an ambiguous OS failure.

## AP-R4-LOG-017 — Optional asynchronous dispatch decision

- Status: Draft
- Dependencies: `AP-R4-LOG-004`; `AP-R4-LOG-007`; `AP-R4-LOG-015`; `AP-R4-LOG-016`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: The component design shall document whether each binding uses synchronous or asynchronous delivery and justify any worker queue by blocking-I/O behavior or measurements; any asynchronous path shall preserve LOG-015/016 bounds and ordering and shall not wait in the logging caller for queue space or sink completion.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.6, PDF pp. 37, SWS_LOG_00095 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_017_DispatchDecision`: Review the decision and, if asynchronous delivery is selected, use a blocked fake transport to verify enqueue/full-queue behavior and fixed memory bounds; a synchronous decision needs no worker.
- Unit Tests:
- Deviation: The source discusses high-load buffering, not a mandatory thread or non-blocking architecture. The worker queue is optional and distinct from unavailable-destination buffering; oldest eviction is the selected project policy for any shared pending queue. No hard realtime timing guarantee is claimed.

## AP-R4-LOG-018 — Delivery ownership and concurrency

- Status: Draft
- Dependencies: `AP-R4-LOG-001`; `AP-R4-LOG-002`; `AP-R4-LOG-007`; `AP-R4-LOG-015`; `AP-R4-LOG-016`; `AP-R4-LOG-017`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: The framework shall own configured sinks, buffers and any workers for their delivery lifetime, serialize access to each file stream, and preserve complete records across concurrent callers; queued record storage shall remain valid until delivered or discarded.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.6, PDF pp. 22, SWS_LOG_00005](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_018_DeliveryLifetime`: Use deterministic concurrent caller and lifetime fixtures with fake sinks; check record integrity, owned queued data, single file writer and worker-free behavior when synchronous.
- Unit Tests:
- Deviation: Sink/worker ownership extends source logger ownership as a project decision. Shared cross-process files and public logger deregistration are excluded; no later shutdown implementation is needed for the finite fixture lifetime checks.

## AP-R4-LOG-019 — Orderly dispatcher shutdown

- Status: Draft
- Dependencies: `AP-R4-LOG-004`; `AP-R4-LOG-014`; `AP-R4-LOG-015`; `AP-R4-LOG-016`; `AP-R4-LOG-017`; `AP-R4-LOG-018`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Orderly platform logging shutdown shall stop accepting new records, attempt to deliver retained records to configured destinations and flush file destinations, then release workers/resources within a documented finite shutdown budget; undeliverable records at budget expiry shall be discarded with internally observable loss status.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.1.2, PDF pp. 17–18, SWS_LOG_00123 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_019_Shutdown`: Use fake sinks/clock for empty shutdown, drain order, final flush, blocked transport, expiry and repeated shutdown; verify rejected post-shutdown calls cause no sink access.
- Unit Tests:
- Deviation: The finite deadline and loss status deliberately qualify the source all-queued-message shutdown intent. No unconditional delivery promise, core Deinitialize API, forced cancellation of arbitrary blocked OS calls or abrupt-crash flush is implied. Selected transports must support bounded teardown; applications stop logging before shutdown.

## AP-R4-LOG-020 — Separate application/component streams

- Status: Draft
- Dependencies: `AP-R4-LOG-006`; `AP-R4-LOG-007`; `AP-R4-LOG-010`; `AP-R4-LOG-012`; `AP-R4-LOG-013`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Platform configuration shall support distinct file streams for application/component identities; records routed to one configured stream shall not appear in another unless explicitly selected for that destination.
- AUTOSAR source: [Specification of Log and Trace, AUTOSAR AP R23-11, §7.2.1, 7.2.4, PDF pp. 18–21, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- Verification: `AP_R4_LOG_020_SeparatedStreams`: Route two synthetic identities through fake/temporary file destinations and verify separation, selected fan-out and independent rotation bounds without a second managed application.
- Unit Tests:
- Deviation: Per-application files and component stream keys are project choices. This does not expand the one-application manager scope or mandate a final per-application sink policy; filename safety and mapping are recorded component decisions.

## AP-R4-LOG-021 — Minimal global lifecycle log

- Status: Draft
- Dependencies: `AP-R4-LOG-002`; `AP-R4-LOG-004`; `AP-R4-LOG-007`; `AP-R4-LOG-012`; `AP-R4-LOG-013`; `AP-R4-LOG-020`; `AP-R4-EXEC-018`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: A bounded global lifecycle stream shall retain selected manager start/stop, startup failure, unexpected termination, restart attempt and restart outcome events with application identity and known execution generation/reason, while ordinary application messages shall not be copied to it.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_021_GlobalLifecycle`: Replay EXEC-018 event fixtures through a capturing destination to verify event selection/order/metadata and exclusion of ordinary messages; check independent storage bounds and unchanged manager decisions on sink failure.
- Unit Tests:
- Deviation: Also depends on AP-R4-EXEC-018, an earlier component stage. The selected mechanism and paths remain platform design choices; no centralized collector, multi-application supervisor, persistent diagnostic/health store or mandatory major-sink-failure log is introduced. Failure reporting must not recurse.

## AP-R4-LOG-022 — Logging verification and documented policy

- Status: Draft
- Dependencies: `AP-R4-LOG-005`; `AP-R4-LOG-006`; `AP-R4-LOG-007`; `AP-R4-LOG-008`; `AP-R4-LOG-009`; `AP-R4-LOG-010`; `AP-R4-LOG-011`; `AP-R4-LOG-012`; `AP-R4-LOG-013`; `AP-R4-LOG-014`; `AP-R4-LOG-015`; `AP-R4-LOG-016`; `AP-R4-LOG-017`; `AP-R4-LOG-018`; `AP-R4-LOG-019`; `AP-R4-LOG-020`; `AP-R4-LOG-021`; `AP-R4-EXEC-021`
- Owning issue: [#73](https://github.com/Romanr92/adaptive-pi/issues/73)
- Requirement: Release evidence shall record named host checks for each applicable logging contract, ARM64 build results and QEMU operation of the selected Linux bindings, including file rotation and global lifecycle events during the crash/restart/controlled-stop scenario; documentation shall identify actual sink selection, limits, flush/loss behavior and explicitly deferred deployment policy.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_LOG_022_LoggingEvidence`: Record tested revision, both exception-mode CI results, exceptions-OFF local results, image/dependency versions, target paths/output and feasibility or conditional exclusions. Review the mapping from every LOG ID to observed checks.
- Unit Tests:
- Deviation: Also depends on AP-R4-EXEC-021, an earlier component stage. Queue/worker and journald checks apply only to adopted paths; omitted paths need recorded decisions. Fakes avoid host journald dependencies. Defaults, future manifest configuration, persistent partition placement, retention and DLT/network/diagnostic integration remain open. This is later evidence, not a prerequisite for earlier unit-level completion.

