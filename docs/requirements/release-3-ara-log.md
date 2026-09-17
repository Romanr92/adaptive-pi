# Release 3 - clean-room ara::log-inspired requirements

## Scope

Release 3 provides an AUTOSAR-inspired logging framework for host and QEMU
development.

It includes non-modelled stream logging, manually authored modelled-message
definitions, and compile-time trace routing. AdaptivePi does not generate C++
definitions or routing configuration from ARXML.

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
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.6, p. 22,
  SWS_LOG_00005; §8.2.1, p. 54, SWS_LOG_00021; §8.3.2, p. 68,
  SWS_LOG_00172](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_003_CreateLoggerOwnsLogger`.
- Deviation: Release 3 has no logger deregistration or platform lifecycle
  management. Logger ownership shall be isolated behind the logging framework
  so lifecycle registration can be added later.

## AP-R3-LOG-004 - Console sink

- Status: Approved
- Requirement: A created logger shall use the console sink.
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
  later.

## AP-R3-LOG-006 - AdaptivePi console record format

- Status: Approved
- Requirement: The console sink shall format each submitted record exactly as:

  `[TIMESTAMP][PID][TID][LEVEL][CONTEXT] message\n`

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
  non-verbose DLT message transmission are out of scope.

## AP-R3-LOG-011 - Compile-time trace routing

- Status: Approved
- Requirement: Modelled messages shall support compile-time routing to the
  logger, a trace artifact interface, both, or neither.
- Requirement: The trace artifact interface shall expose
  `ara::log::ext::TraceArti(const MsgId&, const Params&...) noexcept`.
- Requirement: The logger shall invoke `TraceArti` only for a modelled message
  configured for trace routing.
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