# Release 4 — Execution Management requirements

Release 4 supervises one application through a private systemd adapter and a
project-owned manager. Recoverable errors use the Release 3 Result/ErrorCode
contracts in both compiler exception modes. These entries define a reduced
manager API; they do not implement full ara::exec or State Management.

## Requirement order

Read the explicit stages below, also listed in [the Release 4 overview](release-4-overview.md):
EXEC-001–008; supplemental controller contracts EXEC-022–024; EXEC-009–017;
timeout cleanup EXEC-025; history EXEC-018; lifecycle milestones EXEC-026 and
event timing EXEC-027; then the demonstration EXEC-019–021. Existing IDs are
preserved. Every dependency is a lower-numbered entry or an explicitly earlier
component stage; the supplemental entries do not depend on the demonstration.
Each planned check verifies only its own observable contract and earlier prerequisites;
later integration evidence does not block completion of an earlier requirement.

## AP-R4-EXEC-001 — Execution error domain

- Status: Draft
- Dependencies: Existing Release 3 core/log contracts only; no Release 4 prerequisite.
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: Execution operations shall provide a project-owned error domain and enum distinguishing invalid configuration/state, backend unavailable, start/stop/query failure, operation timeout and resource failure; public recoverable failures shall use the existing Result and ErrorCode contracts in both exception configurations.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.3.1, PDF pp. 36–37, SWS_EM_02547, SWS_EM_02548](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 EXP_InterfacesGuidelines, §3.1–3.1.2.2, PDF pp. 8–10, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
- Verification: `AP_R4_EXEC_001_ErrorDomain` verifies distinct categories, domain identity and guarded conversion; common explicit Result handling runs in both modes.
- Unit Tests:
- Deviation: The project enum is not the complete AUTOSAR ExecErrc API; no StateClient/Future API is required.

## AP-R4-EXEC-002 — Controller observations

- Status: Draft
- Dependencies: `AP-R4-EXEC-001`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The process-control boundary shall return an observation distinguishing inactive, activating, active, deactivating and failed, together with an opaque execution-generation identifier when available, an exit status/signal when known, and explicit available/unavailable confirmation that the managed execution has no remaining process or pending activation. A query error shall remain an ErrorCode rather than being reported as an inactive process; a failed unit status alone shall not confirm process absence.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.2, PDF pp. 41–42, SWS_EM_01002, SWS_EM_01003, SWS_EM_01402, SWS_EM_01404, SWS_EM_01006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_002_Observations` checks each status, absent exit information, generation changes, confirmed versus unavailable execution absence, pending activation and query-error distinction.
- Unit Tests:
- Deviation: Statuses reflect the systemd boundary; they are not AUTOSAR ExecutionState values. Generations prevent one failure being handled repeatedly.

## AP-R4-EXEC-003 — Controller interface and fake

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The controller interface shall offer prepare, start, stop and observe operations for one validated MAN-014 configuration; a scripted fake shall implement these operations and record requests without requiring systemd on the host.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_003_FakeController` verifies scripted results, recorded arguments, operation ordering and backend error propagation.
- Unit Tests:
- Deviation: prepare is side-effect-limited unit preparation; it shall not launch the application. No arbitrary shell-command API.

## AP-R4-EXEC-004 — Systemd unit preparation

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-MAN-015`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`; `AP-R4-EXEC-003`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The systemd adapter shall create or select the configuration-named service unit for the validated package with the validated executable, argument vector defined by MAN-015, working directory and environment overrides, and with automatic systemd restart disabled.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.3, PDF pp. 50, SWS_EM_01012, SWS_EM_01072, SWS_EM_01078](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.4, PDF pp. 50–51, SWS_EM_02246](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_004_UnitPreparation` checks generated/selected unit settings, exact executable argument zero and subsequent launch values, empty argument lists and Restart=no; existing selected units that conflict with the configuration shall fail preparation.
- Unit Tests:
- Deviation: Use the defined package or a documented transient/runtime unit. systemd owns process creation; AdaptivePi owns restart decisions. No general service installer or arbitrary unit execution.

## AP-R4-EXEC-005 — Safe systemd invocation

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The adapter shall invoke systemd through structured D-Bus calls or fixed commands with separate arguments, without interpolating manifest strings into a shell command; it shall preserve launch-value semantics and bound each backend operation with a documented configurable positive timeout.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_005_Invocation` checks shell metacharacters remain data, fixed backend operation selection, argument preservation and deterministic timeout errors using adapter doubles.
- Unit Tests:
- Deviation: D-Bus versus fixed command transport is left to implementation; timeout values are adapter configuration, not new manifest fields. Operating-system APIs are not bounded-proved.

## AP-R4-EXEC-006 — Backend start

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The adapter start operation shall submit one start request for the prepared unit and return success only for an accepted backend request, mapping rejection, unavailable backend and timeout to execution errors.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_006_Start` checks accepted/rejected requests and each backend failure using doubles; later QEMU verification exercises the real adapter.
- Unit Tests:
- Deviation: Request acceptance is not proof of application readiness; observations establish running status later.

## AP-R4-EXEC-007 — Backend stop

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The adapter stop operation shall request controlled termination of the prepared unit and map backend rejection, unavailable backend and timeout to execution errors.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.1.2, PDF pp. 39, SWS_EM_01055](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_007_Stop` checks correct target, accepted/rejected stop and failure categories with adapter doubles.
- Unit Tests:
- Deviation: systemd implements signal delivery; the supplemental termination contracts define its required initial signal and forced-termination capability. No application-facing signal API or AUTOSAR termination-handler compatibility is required.

## AP-R4-EXEC-008 — Backend active-state observation

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`; `AP-R4-EXEC-006`; `AP-R4-EXEC-007`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The adapter observe operation shall translate systemd state, execution generation and available exit information into EXEC-002 observations without starting, stopping or restarting a unit.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.2, PDF pp. 41–42, SWS_EM_01002, SWS_EM_01003, SWS_EM_01402, SWS_EM_01404, SWS_EM_01006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_008_Observe` checks translation of active, transitional, inactive and failed states, pending activation, process-absence confirmation and unavailable backend information.
- Unit Tests:
- Deviation: Observation is explicit polling; no mandatory asynchronous callback, PHM channel or real host systemd in unit tests.

## AP-R4-EXEC-022 — Initial signal mask for non-reporting applications

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: Every managed application shall enter its initial application code with an empty blocked-signal set; generated and selected units shall preserve that launch condition. Preparation shall reject a selected unit whose effective launch behavior conflicts with it.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.4.2, PDF pp. 49–50, SWS_EM_02579 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_022_InitialSignalMask` checks compatible/conflicting launch conditions through adapter doubles. A later QEMU startup probe shall inspect the actual blocked-signal set before installing application signal handling, on both initial start and restart; unit settings alone are not target evidence.
- Unit Tests:
- Deviation: Release 4 applications use the non-reporting-process subset and own subsequent signal handling. The reporting-process mask in SWS_EM_02578 and ExecutionClient are excluded. The systemd mechanism remains an implementation choice.

## AP-R4-EXEC-023 — SIGTERM-first controlled termination

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`; `AP-R4-EXEC-007`; `AP-R4-EXEC-008`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: An accepted ordinary stop of a live managed execution shall initiate termination with SIGTERM, including while activation is pending. Generated and selected units shall preserve this effective behavior; preparation shall reject settings or stop actions that substitute another initial termination signal. An execution already confirmed absent shall not require signal delivery.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.1.2, PDF pp. 39, SWS_EM_01055 and surrounding termination notes](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_023_TerminationSignal` checks effective generated/selected settings, conflicting overrides, pending activation and already-absent execution through doubles. A later QEMU probe shall record receipt of SIGTERM and graceful exit under the real adapter.
- Unit Tests:
- Deviation: systemd delivers signals; request acceptance is distinct from confirmed exit. The ExecutionClient termination-handler API SWS_EM_02577 {DRAFT} is excluded. Explicit force-termination is a separate operation, not an ordinary stop.

## AP-R4-EXEC-024 — Bounded force-termination boundary

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`; `AP-R4-EXEC-003`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`; `AP-R4-EXEC-007`; `AP-R4-EXEC-008`; `AP-R4-EXEC-023`
- Owning issue: [#19](https://github.com/Romanr92/adaptive-pi/issues/19)
- Requirement: The controller and scripted fake shall support a force-termination request for the bound application. The real adapter shall cancel pending activation work and request SIGKILL for any remaining managed execution through systemd, without queuing another start. The operation shall use a documented positive backend timeout and return an execution error on rejection, backend unavailability or timeout; success shall mean request acceptance, with execution absence confirmed separately through observation.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.5.5, PDF pp. 70–72, SWS_EM_02255, SWS_EM_02310, SWS_EM_02312](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf), contextual timeout-cleanup behavior within Function Group transitions.
- Verification: `AP_R4_EXEC_024_ForceTermination` verifies bound target, pending-start cancellation, force request, absent execution, rejected/unavailable/timed-out operations and no duplicate start with doubles/fake. Later QEMU evidence shall exercise a test process that ignores SIGTERM and confirm forced exit and no delayed activation.
- Unit Tests:
- Deviation: This project extension supports the existing manager deadlines; it is not the excluded Function Group algorithm or a public arbitrary-signal interface. A successful request does not guarantee OS termination. Cancellation plus process-absence confirmation prevents a late backend job from reviving a timed-out execution; transport and cancellation details remain private adapter decisions.

## AP-R4-EXEC-009 — Manager state model

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-002`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The manager shall represent stopped, initializing, running, terminating, failed and restarting, expose its current state and restart-attempt count, and begin stopped with zero attempts.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.2, PDF pp. 41–42, SWS_EM_01002, SWS_EM_01003, SWS_EM_01402, SWS_EM_01404, SWS_EM_01006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_009_StateModel` verifies all state values and initial state without any controller.
- Unit Tests:
- Deviation: stopped is an explicit AdaptivePi addition to the five issue #20 states, so pre-start and successful shutdown have a defined endpoint. These request/operation states are not AUTOSAR Process States or the two-value AUTOSAR ExecutionState enum; the process-state citations are contextual.

## AP-R4-EXEC-010 — Permitted transition relation

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-009`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The pure transition relation shall admit stopped→initializing; initializing→running/failed/terminating; running→failed/terminating; terminating→stopped/failed; failed→restarting/initializing/terminating; restarting→running/failed/terminating. Every other edge shall be rejected without mutating state; a repeated observation may leave state unchanged without recording a transition.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_010_Transitions` enumerates all ordered pairs and checks permitted edges and rejected-state preservation without backend operations.
- Unit Tests:
- Deviation: A project state graph replaces machine/function-group state coordination. Restart eligibility is separate from permission for the failed→restarting edge.

## AP-R4-EXEC-011 — Restart decision and episode budget

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The pure restart decision shall allow another automatic start only for an unexpected termination of an execution previously confirmed running, when mode is on_failure and attempts_used<max_attempts; each issued restart shall consume one attempt before its backend result is known. A confirmed running observation shall not reset the budget; a new explicit start episode shall reset it.
- AUTOSAR source: [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.2, PDF pp. 385–386, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_011_RestartDecision` covers both modes, controlled versus unexpected termination, exit before running confirmation, zero/exhausted budget, boundary counts and episode reset without backend operations.
- Unit Tests:
- Deviation: This project crash policy differs from AUTOSAR startup retry configuration. No persistence, delay/backoff or infinite retries.

## AP-R4-EXEC-012 — Manager configuration and serialized use

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-001`; `AP-R4-EXEC-003`; `AP-R4-EXEC-008`; `AP-R4-EXEC-022`; `AP-R4-EXEC-023`; `AP-R4-EXEC-024`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-011`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: A manager instance shall accept one validated configuration and controller, prepare that application before accepting Start, reject replacement while its state is not stopped, and serialize its command/observation handling through one documented execution context.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_012_Configuration` checks preparation failure, valid binding, replacement restriction and documented single-context use with the fake.
- Unit Tests:
- Deviation: Thread-safe concurrent public commands, dependency graphs and multiple applications are excluded; callers serialize access.

## AP-R4-EXEC-013 — Explicit manager startup

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-006`; `AP-R4-EXEC-008`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-011`; `AP-R4-EXEC-012`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: Start from stopped or failed shall require observation confirming that the bound execution has no remaining process or pending activation, then reset the episode budget, record initializing, and issue one backend start; rejection shall record failed and return an execution error. Unconfirmed absence or commands from other states shall fail without a backend start.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_013_Startup` checks allowed/disallowed states, confirmed absence, an already-active selected unit, pending activation, observation failure, episode reset and startup rejection with the fake.
- Unit Tests:
- Deviation: An unsuccessful initial start is observable but is not automatically retried by the crash policy.

## AP-R4-EXEC-014 — Running confirmation and deadlines

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-002`; `AP-R4-EXEC-008`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The manager shall record running only after an active observation of the started execution before its deadline; activating shall retain initializing/restarting. Configurable positive startup deadlines measured with an injected monotonic clock from submission of the backend start shall bound pending initialization/restart; backend-call time shall not reset or postpone them. Expiry shall record failed and return an operation-timeout error, including when observation is unavailable.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.2, PDF pp. 42, SWS_EM_01402](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 TPS_ManifestSpecification, §8.3.1, PDF pp. 392, TPS_MANI_01277, TPS_MANI_01278 {both DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_TPS_ManifestSpecification.pdf)
- Verification: `AP_R4_EXEC_014_ConfirmationAndTimeout` checks pending, active, early termination and exact deadline boundaries with fake controller and clock; query failure shall not fabricate running or elapsed time, and shall not postpone expiry established by the monotonic clock. Activity first observed at or after expiry shall not complete that timed-out operation successfully.
- Unit Tests:
- Deviation: Backend active means started, not a received ara::exec kRunning report or business readiness. Deadline values are manager configuration; startup logging is separately verified in the application.

## AP-R4-EXEC-015 — Controlled manager shutdown

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-007`; `AP-R4-EXEC-008`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`; `AP-R4-EXEC-014`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: Stop while initializing/running/restarting/failed shall record terminating and controlled-stop intent before issuing the backend stop; execution absence confirmed before the stop deadline shall record stopped. A configurable positive stop deadline measured with the same injected monotonic clock from submission of the backend stop shall record failed and return timeout when termination is not confirmed in time, including when observation is unavailable; backend-call time shall not reset or postpone it. The manager shall clear pending automatic restart work on Stop and shall never restart an execution terminated under that intent.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.1.2, PDF pp. 39, SWS_EM_01055](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_015_ControlledStop` checks stop from each allowed state, absence confirmation, stop errors, observation failure, exact deadline and late-inactive boundaries, and crash observation during termination; Stop from stopped is idempotent.
- Unit Tests:
- Deviation: A stop failure leaves observable failed outcome and preserves no-restart intent until a new explicit Start. Successful shutdown has the project stopped state.

## AP-R4-EXEC-016 — Unexpected termination detection

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-002`; `AP-R4-EXEC-008`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`; `AP-R4-EXEC-014`; `AP-R4-EXEC-015`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: An inactive/failed observation confirming execution absence for an execution previously starting or running without controlled-stop intent shall record failed and capture its known exit information exactly once per execution generation. A failed observation without confirmed absence shall expose the backend failure without asserting termination or authorizing a restart; unknown observation/query error alone shall not be treated as a crash.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.2, PDF pp. 41–42, SWS_EM_01002, SWS_EM_01003, SWS_EM_01402, SWS_EM_01404, SWS_EM_01006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_016_UnexpectedTermination` checks early exit, crash, clean self-exit, repeated/stale generations, missing exit data, failed-but-live status, unavailable absence, pending activation and query failures with the fake.
- Unit Tests:
- Deviation: Even exit status zero is unexpected for this long-running service unless Stop was requested. systemd invocation identity or an equivalent adapter token identifies a generation.

## AP-R4-EXEC-017 — Automatic restart execution

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-006`; `AP-R4-EXEC-008`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-011`; `AP-R4-EXEC-012`; `AP-R4-EXEC-014`; `AP-R4-EXEC-015`; `AP-R4-EXEC-016`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: After a detected unexpected termination, the manager shall apply EXEC-011; when eligible and execution absence is confirmed with no pending activation it shall record restarting, consume one attempt and issue one backend start. It shall record running after active confirmation before the startup deadline or failed after rejection, early termination or timeout. It shall not retry a rejected restart request or a restart that exits before running confirmation; a later unexpectedly terminated successfully restarted execution may consume remaining budget.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 SWS_StateManagement, §7.11.1, PDF pp. 42–43, no requirement ID for this explanatory passage](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_StateManagement.pdf)
- Verification: `AP_R4_EXEC_017_RestartExecution` checks success, rejection, early termination without another retry, timeout, exhausted/disabled policy, repeated failure generations and absent/live/unavailable execution with pending-activation checks using the fake.
- Unit Tests:
- Deviation: Local restart supervision combines a small project recovery policy with execution control; it is not AUTOSAR State Management/PHM recovery or machine reset.

## AP-R4-EXEC-025 — Cleanup after a lifecycle deadline expires

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-002`; `AP-R4-EXEC-008`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`; `AP-R4-EXEC-014`; `AP-R4-EXEC-015`; `AP-R4-EXEC-016`; `AP-R4-EXEC-017`; `AP-R4-EXEC-024`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: On expiry of an initialization, restart or stop deadline, the manager shall retain the failed/timeout outcome, cancel pending automatic restart work, mark the execution unresolved and submit one EXEC-024 cleanup request before returning the timeout. Until observation confirms no remaining process or pending activation, it shall reject another explicit or automatic start and configuration replacement, and shall not promote late activity to running or classify cleanup termination as a new crash. A documented positive cleanup-confirmation deadline measured with the injected monotonic clock from cleanup-request submission shall bound the pending cleanup episode; backend-call time shall not reset or postpone it. Request or confirmation failure shall leave absence unresolved and observable, without automatic retry loops. Successful cleanup shall clear the unresolved condition while retaining failed and the original timeout until a later explicit Start or Stop is handled.
- AUTOSAR source: [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.5.5, PDF pp. 70–72, SWS_EM_01065, SWS_EM_02255, SWS_EM_02258, SWS_EM_02253, SWS_EM_02260, SWS_EM_02310, SWS_EM_02312](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf), contextual source for deadline reactions and cleanup ordering.
- Verification: `AP_R4_EXEC_025_TimeoutCleanup` uses fake controller/clock cases for each expired operation, exact boundaries, cleanup request before timeout return, late activity, pending activation, unknown query/generation, force rejection/timeout, cleanup-confirmation expiry and later confirmed absence; verify no duplicate start, no budget reset/extra attempt, no crash-based retry and blocked reconfiguration while unresolved.
- Unit Tests:
- Deviation: Function Groups, StateClient reporting and the source startup-retry algorithm remain excluded. AUTOSAR non-reporting processes have no application-readiness wait; these are project backend-observation deadlines. Cleanup is requested before returning the error, but confirmed OS termination may follow later. Initial-start failures and unsuccessful restarts are not automatically retried. An expired cleanup deadline bounds waiting rather than proving a process dead; later explicit observation or Stop may resolve it. Cleanup success does not add a failed→stopped edge to EXEC-010.
- Later use: Release 5 [#27](https://github.com/Romanr92/adaptive-pi/issues/27) needs accurate provider-loss/restart outcomes, and Release 6 [#31](https://github.com/Romanr92/adaptive-pi/issues/31) needs to distinguish an operation timeout from a confirmed exit. Reconnect, diagnostics transport and persistence remain later-release work.

## AP-R4-EXEC-018 — Observable transition and restart history

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-009`; `AP-R4-EXEC-010`; `AP-R4-EXEC-011`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`; `AP-R4-EXEC-014`; `AP-R4-EXEC-015`; `AP-R4-EXEC-016`; `AP-R4-EXEC-017`; `AP-R4-EXEC-025`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The manager shall expose ordered transition/restart events with application ID, previous/new state, execution generation when known, attempt count, reason and known exit/backend error; the same events shall be eligible for emission through existing ara::log. Failure to emit a log shall not change lifecycle or restart decisions.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_018_Events` checks exact event order and metadata for startup, stop, crash, restart success/failure, timeout/cleanup outcomes and suppression; event/log failure shall not trigger another restart.
- Unit Tests:
- Deviation: History is in-memory with a documented finite capacity and explicit oldest-event eviction; no persistent health/diagnostic records. Event retention and logging are observations, not control dependencies.

## AP-R4-EXEC-026 — Distinct request and observed lifecycle milestones

- Status: Draft
- Dependencies: `AP-R4-EXEC-002`; `AP-R4-EXEC-008`; `AP-R4-EXEC-018`; `AP-R4-EXEC-025`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: The bounded EXEC-018 history shall distinguish start/restart request and its backend acceptance/rejection, first active observation of that execution, ordinary/forced termination request and its backend outcome, and first confirmed execution absence. It shall retain deadline expiry and cleanup confirmation/failure as distinct outcomes. Request events shall not assert process creation, signal receipt or exit; unavailable generation, exit or absence information shall remain explicitly unavailable. Repeated observations shall not duplicate a milestone for the same execution, and stale observations shall not change the current execution's milestones.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.2, PDF pp. 18, RS_EM_00152 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf); [AUTOSAR AP R23-11 SWS_ExecutionManagement, §7.4.3, PDF pp. 43, SWS_EM_02573–SWS_EM_02576 {all DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf)
- Verification: `AP_R4_EXEC_026_LifecycleMilestones` checks ordered requests/results/confirmations through fake-controller scripts, immediate rejection, delayed activation/exit, unknown metadata, repeated/stale observations and cleanup. Captured ara::log payloads shall distinguish a request from an observed outcome without affecting control decisions.
- Unit Tests:
- Deviation: The source traces actual process creation, received kRunning, termination request and actual termination using DLT. This systemd-polled subset records what its boundary can establish; observation of active is neither creation time nor a kRunning report, and observation of absence is not an exact exit-time measurement. DLT, ExecutionClient and standardized DltMessage types are excluded. Extra request/result/cleanup events are project extensions retained within the existing finite history bound.
- Later use: Release 6 [#31](https://github.com/Romanr92/adaptive-pi/issues/31) can read meaningful lifecycle/crash history with explicit unavailable data. Freeze-frame selection, delivery and persistence are deferred; no diagnostic service is needed to verify these events.

## AP-R4-EXEC-027 — Original lifecycle event occurrence time

- Status: Draft
- Dependencies: `AP-R4-EXEC-014`; `AP-R4-EXEC-018`; `AP-R4-EXEC-026`
- Owning issue: [#20](https://github.com/Romanr92/adaptive-pi/issues/20)
- Requirement: Each retained manager event shall capture its occurrence time from the injected monotonic clock when the manager handles the request, result, observation or deadline event; history reads and delayed log emission shall preserve that value. The event metadata shall identify the manager timing episode and clock unit/origin so values are interpretable within that episode and are not represented as UTC, globally synchronized time or the exact OS process-event time. Log emission shall carry original event timing separately from the logging record's submission timestamp.
- AUTOSAR source: No direct AUTOSAR requirement source mandates this event field; this is an AdaptivePi foundation decision. Timing-analysis rationale is provided by [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.2, PDF pp. 18, RS_EM_00152 {DRAFT}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf) and [SWS_ExecutionManagement, §7.4.3, PDF pp. 43, explanatory tracing context](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_ExecutionManagement.pdf).
- Verification: `AP_R4_EXEC_027_EventOccurrenceTime` advances a controlled clock between event capture, history reads and logging; verifies unchanged original times, episode metadata, same-tick ordering from history order, and distinct log submission timestamps. No real sleep, synchronized clock or future diagnostic consumer is required.
- Unit Tests:
- Deviation: Timestamp representation and episode identity are project choices; no shared cross-process monotonic origin, persistent time identity across reboot or synchronization service is promised. R3-LOG-007 and R4-LOG-002 continue to sample logging metadata at submission; event time is additional history/payload metadata.
- Later use: Release 6 [#30](https://github.com/Romanr92/adaptive-pi/issues/30) and [#31](https://github.com/Romanr92/adaptive-pi/issues/31) need original failure context when freeze frames are collected later. UTC correlation, persistent diagnostic timestamps and transport are later additions rather than prerequisites for this bounded in-memory foundation.

## AP-R4-EXEC-019 — Supervised demonstration package

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-004`; `AP-R4-EXEC-005`; `AP-R4-EXEC-006`; `AP-R4-EXEC-007`; `AP-R4-EXEC-008`; `AP-R4-EXEC-012`; `AP-R4-EXEC-013`; `AP-R4-EXEC-014`; `AP-R4-EXEC-015`; `AP-R4-EXEC-016`; `AP-R4-EXEC-017`; `AP-R4-EXEC-018`; `AP-R4-EXEC-022`; `AP-R4-EXEC-023`; `AP-R4-EXEC-024`; `AP-R4-EXEC-025`; `AP-R4-EXEC-026`; `AP-R4-EXEC-027`
- Owning issue: [#21](https://github.com/Romanr92/adaptive-pi/issues/21)
- Requirement: platform-test-service shall be a target-compatible long-running application packaged according to the manifest baseline with a unit whose lifecycle settings agree with the validated configuration, and shall emit a successful startup record through ara::log on each execution.
- AUTOSAR source: [AUTOSAR AP R23-11 RS_ManifestSpecification, §3.1.1.2, PDF pp. 14, RS_MANI_00006 {DRAFT in AUTOSAR}](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ManifestSpecification.pdf); [AUTOSAR AP R23-11 RS_ExecutionManagement, §4.2.3, PDF pp. 19, RS_EM_00103](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_RS_ExecutionManagement.pdf)
- Verification: Package review, application tests and later `AP_R4_EXEC_019_TargetStartup` QEMU integration check for manifest selection, actual argv[0]/argument preservation, initial signal mask, configured launch values and startup record. Small test probes may establish launch conditions without expanding the public application API.
- Unit Tests:
- Deviation: Target compatibility extends the existing host-only demonstrator; no new logging API or generated AUTOSAR application.

## AP-R4-EXEC-020 — Controlled test-only fault injection

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-019`
- Owning issue: [#21](https://github.com/Romanr92/adaptive-pi/issues/21)
- Requirement: The demonstration service shall provide an explicitly enabled, documented test-only trigger causing abnormal termination after startup, with normal operation leaving the trigger disabled; its controlled-stop path shall exit cleanly under systemd termination.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: Application tests plus later `AP_R4_EXEC_020_FaultInjection` QEMU checks for disabled trigger, enabled crash, distinct failure evidence and clean shutdown.
- Unit Tests:
- Deviation: This is deliberate test instrumentation, not a production fault-injection or diagnostics interface.

## AP-R4-EXEC-021 — QEMU package deployment and scenario

- Status: Draft
- Dependencies: `AP-R4-MAN-014`; `AP-R4-EXEC-019`; `AP-R4-EXEC-020`
- Owning issue: [#21](https://github.com/Romanr92/adaptive-pi/issues/21)
- Requirement: The project shall document and provide a reproducible QEMU deployment/scenario workflow that installs the demonstration package and its required manager/unit configuration, starts it from the manifest, waits for manager running confirmation before triggering one fault, and records startup, failed/restarting transitions and successful automatic restart, followed by a controlled stop without restart.
- AUTOSAR source: No direct AUTOSAR requirement source; this is an AdaptivePi design/verification decision.
- Verification: `AP_R4_EXEC_021_QemuCrashRestart` integration procedure shall record commands, target paths, exit results, application/generation IDs, request/observed milestones and original event times; include SIGTERM graceful-stop and separate ignored-SIGTERM/forced-cleanup probe evidence, with no late activation or duplicate execution.
- Unit Tests:
- Deviation: Deploy the actual package/unit paths rather than only an unrelated SSH-run binary; deployment is a developer workflow, not general update/installation management.

