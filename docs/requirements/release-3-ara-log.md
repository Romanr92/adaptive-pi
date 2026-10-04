# Release 3 - clean-room ara::log-inspired requirements

## Scope

Release 3 provides an AUTOSAR-inspired logging framework for host and QEMU
development.

It includes non-modelled stream logging, manually authored modelled-message
definitions, and compile-time trace routing. AdaptivePi does not generate C++
definitions or routing configuration from ARXML.

## Contract decisions and issue allocation

The clarifications below are approved AdaptivePi design decisions adopted on
2026-10-04. They do not assert additional AUTOSAR compatibility. Rationale and
implementation guidance are in [ADR 0007](../adr/0007-release-3-logging-contracts.md).

- Issue #3 implements LOG-001 through LOG-004, including concurrent creation,
  owned context storage, reference stability, and creation-failure behavior.
- Issue #15 implements LOG-005 through LOG-009 and LOG-002 threshold filtering.
- Issue #16 implements LOG-010 and LOG-011, including attributes and the
  interaction between threshold filtering and trace routing.
- Issue #5 collects verification evidence for these clarified contracts in both
  supported exception configurations and the ARM64 target build.

## AP-R3-LOG-001 - LogLevel

- Status: Approved
- Requirement: The component shall define `LogLevel` as an `enum class` with
  underlying type `std::uint8_t`.
- Requirement: `LogLevel` shall define these values:

  - `kOff = 0x00`
  - `kFatal = 0x01`
  - `kError = 0x02`
  - `kWarn = 0x03`
  - `kInfo = 0x04`
  - `kDebug = 0x05`
  - `kVerbose = 0x06`

- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.1.1, p. 46,
  SWS_LOG_00018](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Compile-time verification using `static_assert`, plus unit test
  `AP_R3_LOG_001_LogLevelsHaveExpectedValues`.
- Deviation: None.

## AP-R3-LOG-002 - Logger context

- Status: Approved
- Requirement: A logger context shall have a context ID, context description,
  and default log-level threshold.
- Requirement: The caller is responsible for using context IDs that are unique
  within one application process.
- Requirement: The framework shall retain owned copies of context ID and
  description. Their values and the threshold shall remain immutable in Release 3.
- Requirement: Logger output shall admit only defined severity values from
  `kFatal` through the configured threshold, inclusive. A `kOff` threshold,
  `kOff` message severity, or undefined enumeration value shall emit no logger
  record. An undefined creation threshold is a caller precondition violation.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.4-7.2.5, pp. 20-21;
  §7.2.6, p. 22,
  SWS_LOG_00006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test
  `AP_R3_LOG_002_LoggerRetainsContextProperties`, using a valid,
  caller-unique context ID.
- Deviation: Application IDs, manifests, and cross-process registration are out
  of scope. The `CreateLogger()` API shall retain context ID, description, and
  threshold as separate inputs so a manifest-based creation overload can be
  added later without changing this API.

## AP-R3-LOG-003 - Logger creation and ownership

- Status: Approved
- Requirement: `CreateLogger()` shall create and return a logger owned by the
  logging framework. Application code shall not directly construct a logger.
- Requirement: `CreateLogger()` shall return `Logger&`. The returned reference
  shall remain valid until process shutdown, including after further creations.
  Applications shall not copy, move, destroy, or directly construct loggers.
- Requirement: Calls to `CreateLogger()` with caller-unique IDs shall be safe
  when made concurrently. Duplicate IDs violate the caller's uniqueness
  precondition; Release 3 does not specify duplicate-ID behavior.
- Requirement: The project-owned `TryCreateLogger()` operation shall return
  `ara::core::Result<std::reference_wrapper<Logger>, ErrorCode>`. Resource
  exhaustion shall return a project-owned creation error without terminating or
  exposing an exception in either build mode. It shall not publish a partially
  initialized logger or return a successful fallback. Application initialization
  shall decide whether logging is mandatory; supervisor recovery is deferred to
  Release 4.
- Requirement: The reference-returning `CreateLogger()` shall be `noexcept` and
  shall retrieve a context previously created successfully by `TryCreateLogger()`
  with the same ID, description, and threshold. Missing registration or mismatched
  inputs violate this AdaptivePi API's precondition and shall terminate. It shall
  not perform fallible context creation. Logging during static object destruction
  is outside the supported lifetime contract.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.6, p. 22,
  SWS_LOG_00005; §8.2.1, p. 54, SWS_LOG_00021; §8.3.2, p. 68,
  SWS_LOG_00172](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_003_CreateLoggerOwnsLogger`.
- Deviation: Release 3 has no logger deregistration or platform lifecycle
  management. Logger ownership shall be isolated behind the logging framework
  so lifecycle registration can be added later. The recoverable factory and
  pre-registration contract are explicit AdaptivePi deviations: AUTOSAR specifies
  creation through a reference-returning, non-throwing `CreateLogger()` and does
  not define this two-operation protocol.

## AP-R3-LOG-004 - Console sink

- Status: Approved
- Requirement: A created logger shall use the console sink.
- Requirement: The production console sink shall write to standard output.
  Sink replacement and failure injection shall be implementation-private test
  facilities, unavailable through the public application API.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.2.1, pp. 54-55,
  SWS_LOG_00021 and SWS_LOG_00263](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_004_DefaultSinkIsConsole`.
- Deviation: Console is the only Release 3 sink. DLT, file, and remote sinks
  are out of scope. The logger shall write through an internal sink abstraction
  so additional sink types can be added later.

## AP-R3-LOG-005 - Non-modelled stream logging

- Status: Approved
- Requirement: `Logger` shall provide `LogFatal()`, `LogError()`, `LogWarn()`,
  `LogInfo()`, `LogDebug()`, `LogVerbose()`, and
  `WithLevel(LogLevel level)`.
- Requirement: Each of these operations shall return a `LogStream` configured
  with the corresponding severity level.
- Requirement: `LogStream` shall support stream insertion of supported message
  values and shall submit its accumulated record when the stream is flushed or
  reaches the end of its lifetime.
- Requirement: `LogStream::Flush()` shall submit the accumulated record and
  reset the stream for the next record.
- Requirement: `LogStream` shall be non-copyable and move-constructible, with
  move assignment unavailable. A move shall transfer pending-record ownership;
  the moved-from stream shall be usable only for destruction and shall not submit.
- Requirement: A stream with no accumulated message bytes shall submit no record.
  Flush shall clear pending content after submission or discard. Repeated flush
  and destruction after flush shall not submit duplicate records. Further
  insertion after flush shall begin a new record.
- Requirement: Supported insertion values shall comprise `bool`, `char`, integral
  types other than wide character types, `float`, `double`, non-null null-terminated
  character strings, `std::string`, and `std::string_view`. Unsupported types
  shall fail compilation. Booleans shall render as `true` or `false`, characters
  as characters, and integers in decimal. Floating-point values shall use
  locale-independent general formatting with `max_digits10` precision; non-finite
  values shall render as `nan`, `inf`, or `-inf`.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.3.1, pp. 22-26;
  §8.3.1, pp. 57-67; §8.3.2.1-8.3.2.8, pp. 68-71,
  SWS_LOG_00039 to SWS_LOG_00069 and
  SWS_LOG_00131](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit tests
  `AP_R3_LOG_005_SeverityMethodsCreateCorrectStreams`,
  `AP_R3_LOG_005_StreamInsertionBuildsRecord`, and
  `AP_R3_LOG_005_FlushAndDestructionSubmitRecord`.
- Deviation: AdaptivePi supports a documented educational subset of stream
  insertion value types. Unsupported AUTOSAR formatting decorators may be added
  later. Move restrictions and suppression of empty records are AdaptivePi
  policies, not a claim about AUTOSAR special-member requirements. Release 3
  interprets flush as submission followed by a fresh logical record; the R23-11
  flush note also says the buffer is not emptied, so our no-replay behavior is an
  explicit clarification rather than a claim of identical buffer semantics.

## AP-R3-LOG-006 - AdaptivePi console record format

- Status: Approved
- Requirement: The console sink shall format each submitted record exactly as:

  `[TIMESTAMP][PID][TID][LEVEL][CONTEXT] message\n`

- Requirement: TIMESTAMP shall use UTC `YYYY-MM-DDTHH:MM:SS.mmmZ`, truncating
  to milliseconds. PID and TID shall be decimal Linux process and thread IDs.
  LEVEL shall be `FATAL`, `ERROR`, `WARN`, `INFO`, `DEBUG`, or `VERBOSE`.
  CONTEXT shall contain the context ID, not its description.
- Requirement: Context IDs and messages shall escape backslash as `\\`, newline
  as `\n`, and carriage return as `\r`. Context IDs shall additionally escape
  brackets as `\[` and `\]`. Escaping shall occur once at console rendering;
  each record shall end with exactly one physical newline.
- AUTOSAR source: No direct AUTOSAR requirement specifies this exact console
  text format. It is an AdaptivePi decision inspired by AUTOSAR severity,
  context, and timestamp concepts:
  [Specification of Log and Trace, R23-11, §7.2.4, p. 20;
  §8.1.1, p. 46; §7.4, pp. 35-36](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_006_FormatsConsoleRecord`.
- Deviation: The exact textual representation is AdaptivePi-defined rather
  than an AUTOSAR backend format.

## AP-R3-LOG-007 - Runtime metadata providers

- Status: Approved
- Requirement: Each console record shall include a timestamp, process ID, and
  thread ID.
- Requirement: Production logging shall obtain runtime metadata from platform
  providers.
- Requirement: Unit tests shall use controllable stub providers for timestamp,
  process ID, and thread ID so record formatting is deterministic.
- Requirement: Metadata shall be sampled when a record is submitted. TID shall
  identify the submitting thread. Filtered and empty records shall not request
  metadata or write to the sink.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.4, pp. 35-36,
  SWS_LOG_00082 and SWS_LOG_00083](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test
  `AP_R3_LOG_007_FormatsStubbedRuntimeMetadata`.
- Deviation: Timestamp integration follows the AUTOSAR concept. Process ID,
  thread ID, and the provider abstraction are AdaptivePi-defined additions.

## AP-R3-LOG-008 - Logging failure handling

- Status: Approved
- Requirement: If an internal logging or sink failure occurs, the logging
  operation shall not throw an exception or return an error to application code.
  The affected log call shall be discarded.
- Requirement: Record assembly, formatting, metadata acquisition, and sink
  submission failures shall discard the affected record in both exception modes.
  The implementation shall use bounded record storage without throwing allocation
  in the logging path. The maximum accumulated message size shall be 4096 bytes
  before console escaping; overflow shall discard the entire pending record.
  Creation-time allocation is governed separately by LOG-003.
- Requirement: Failure shall be represented internally through explicit status.
  Exception-enabled builds shall contain exceptions from internal providers and
  sinks at the logging boundary; exception-disabled implementations shall use
  non-throwing operations and explicit failure status. No failure shall recursively
  log through the failed path. A failed sink write may already have emitted a
  partial record; Release 3 does not guarantee rollback of console output.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.6, p. 22,
  SWS_LOG_00002](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Component unit test
  `AP_R3_LOG_008_SinkFailureDoesNotThrow`, using an implementation-private test
  sink that fails on write.
- Deviation: The failing sink exists only to verify this requirement.
  Production recovery, persistence, and diagnostics are out of scope. The sink
  abstraction shall preserve failure information internally so diagnostics can
  consume it in a later release.

## AP-R3-LOG-009 - Concurrent logging

- Status: Approved
- Requirement: Logging calls from different threads shall be thread-safe.
- Requirement: Each logging call shall emit one complete record without
  character interleaving with another record.
- Requirement: The ordering of records emitted by different threads is
  unspecified.
- Requirement: Separate streams may share one logger across threads. Concurrent
  mutation of the same stream is outside the contract. Complete-record atomicity
  applies to successful writes through this framework, not unrelated stdout writes.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.2.1, p. 54:
  `CreateLogger()` is reentrant; §8.3.1, pp. 57-67:
  LogStream operations are reentrant](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test
  `AP_R3_LOG_009_ConcurrentRecordsDoNotInterleave`.
- Deviation: AdaptivePi guarantees complete-record atomicity only. It does not
  define a deterministic total order for records emitted by different threads.

## AP-R3-LOG-010 - Modelled messages

- Status: Approved
- Requirement: `Logger` shall provide the modelled-message API
  `Log(const MsgId&, const Params&...)`.
- Requirement: Modelled message definitions shall provide a message identifier,
  severity level, and declared parameter types.
- Requirement: Calling `Logger::Log()` with parameter types that do not match
  the modelled-message definition shall fail during compilation.
- Requirement: `Logger` shall provide `LogWith(...)` to log a modelled message
  together with supported message attributes.
- Requirement: Message definitions shall provide deterministic message rendering
  using the LOG-005 supported value types. Parameter matching shall compare types
  after removal of references and top-level cv qualifiers, without implicit
  numeric or string conversions.
- Requirement: `LogWith(const std::tuple<Attrs...>&, const MsgId&,
  const Params&...) noexcept` shall support typed `Location` (file identifier and
  line number) and `Tag` (string value) attributes. Unsupported attribute types
  shall be rejected at compilation. Release 3 shall accept at most one attribute
  of each type; duplicate types shall be rejected at compilation.
- Requirement: The logger message payload shall start with `id=<message-id>`;
  rendered message text shall follow separated by one space when non-empty.
  Attributes shall follow in tuple order as ` location="file:line"` or
  ` tag="value"`, escaping backslash and double quote within values before
  LOG-006 console escaping. Message IDs shall be non-empty ASCII letters, digits,
  or underscore. Attribute and message rendering together shall obey the
  LOG-008 message-size limit.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.3.2, pp. 26-29,
  SWS_LOG_00240 and SWS_LOG_00241; §8.3.2.9-8.3.2.10, p. 72,
  SWS_LOG_00204 and SWS_LOG_00133](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Compile-time test
  `AP_R3_LOG_010_InvalidModelledMessageParametersDoNotCompile`, plus unit tests
  `AP_R3_LOG_010_LogsModelledMessage` and
  `AP_R3_LOG_010_LogsModelledMessageWithAttributes`.
- Deviation: Modelled-message definitions and routing configuration shall be
  authored manually in C++. ARXML generation, manifests, DLT encoding, and
  non-verbose DLT message transmission are out of scope. Exact normalized type
  matching, duplicate-attribute rejection, and console payload rendering are
  AdaptivePi policies. The attribute model follows R23-11 §7.3.2.4, p. 34;
  the tuple API follows §8.3.2.10, p. 72, SWS_LOG_00133. R23-11 leaves the
  attribute eligibility trait incomplete (`Attr` is TBD).

## AP-R3-LOG-011 - Compile-time trace routing

- Status: Approved
- Requirement: Modelled messages shall support compile-time routing to the
  logger, a trace artifact interface, both, or neither.
- Requirement: The trace artifact interface shall expose
  `ara::log::ext::TraceArti(const MsgId&, const Params&...) noexcept`.
- Requirement: The logger shall invoke `TraceArti` only for a modelled message
  configured for trace routing.
- Requirement: The context threshold shall filter only the logger destination.
  Trace routing shall remain independent of that threshold, including `kOff`.
  With both destinations enabled, logger filtering or failure shall not suppress
  the trace invocation. Trace specializations shall honor their `noexcept`
  contract; recovery from a violating specialization is outside Release 3.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.7.2, pp. 37-38,
  SWS_LOG_20001 to SWS_LOG_20004; Appendix B.1.1, p. 89,
  SWS_LOG_20000](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit tests
  `AP_R3_LOG_011_RoutesToLogger`,
  `AP_R3_LOG_011_RoutesToTraceArtifact`,
  `AP_R3_LOG_011_RoutesToBoth`, and
  `AP_R3_LOG_011_DiscardsDisabledTraceMessage`.
- Deviation: Routing configuration and `TraceArti` specializations are authored
  manually in C++. ARXML generation, external trace-tool integration, and
  runtime trace-configuration changes are out of scope.

## Additional verification for clarified contracts

- LOG-002: `AP_R3_LOG_002_ThresholdFiltersDefinedLevels`, `AP_R3_LOG_002_ContextOwnsInputStrings`.
- LOG-003: `AP_R3_LOG_003_ReferencesRemainStable`, `AP_R3_LOG_003_ConcurrentCreation`, `AP_R3_LOG_003_CreationFailureReturnsError`.
- LOG-005: `AP_R3_LOG_005_EmptyAndRepeatedFlush`, `AP_R3_LOG_005_MoveTransfersPendingRecord`, `AP_R3_LOG_005_SupportedInsertionTypes`, `AP_R3_LOG_005_UnsupportedInsertionDoesNotCompile`.
- LOG-006: `AP_R3_LOG_006_EscapesRecordContent`.
- LOG-007: `AP_R3_LOG_007_SuppressedRecordsSkipProviders`.
- LOG-008: `AP_R3_LOG_008_OverflowDiscardsRecord`, `AP_R3_LOG_008_ProviderFailureDiscardsRecord`, `AP_R3_LOG_008_FormattingFailureDiscardsRecord`, `AP_R3_LOG_008_ThrowingProviderAndSinkAreContained`.
- LOG-009: `AP_R3_LOG_009_SharedLoggerSeparateStreams`.
- LOG-010: `AP_R3_LOG_010_AttributeRendering`, `AP_R3_LOG_010_UnsupportedAndDuplicateAttributesDoNotCompile`, `AP_R3_LOG_010_ExactParameterTypes`, `AP_R3_LOG_010_ModelledMessageSizeLimit`.
- LOG-011: `AP_R3_LOG_011_ThresholdDoesNotFilterTrace`, `AP_R3_LOG_011_LoggerFailureDoesNotSuppressTrace`.

LOG-003 also requires compile-time construction/copy/move restrictions. Failure
injection is private and deterministic; tests shall not exhaust host memory.
Common tests run in both exception modes. Throwing-provider/sink tests run only
in exception-enabled CI; local execution remains exception-disabled. These are
planned checks, not evidence of implementation or successful execution.

Creation verification shall additionally cover successful TryCreateLogger,
CreateLogger retrieval identity, precondition death tests, and rollback after
injected resource failure. Resource failure shall be tested in both modes;
exception-enabled allocation-failure containment is an additional CI check.
