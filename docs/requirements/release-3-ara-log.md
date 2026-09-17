# Release 3 - clean-room ara::log-inspired requirements

## Scope

Release 3 provides logger creation, AUTOSAR-inspired severity levels, structured
message output, and a deterministic local console sink for development and unit
tests.

## AP-R3-LOG-001 - LogLevel

- Status: Approved
- Requirement: The component shall define `LogLevel` with these levels: `Off`,
  `Fatal`, `Error`, `Warn`, `Info`, `Debug`, and `Verbose`.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.1.1, p. 46,
  SWS_LOG_00018](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_001_LogLevelsExist`.
- Deviation: Exact underlying integer values are not part of the AdaptivePi
  contract.

## AP-R3-LOG-002 - Logger context

- Status: Approved
- Requirement: A logger context shall have a context ID, context description,
  and default log-level threshold. The caller is responsible for using context IDs
  that are unique within one application process.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.4-7.2.5, pp. 20-21;
  §7.2.6, p. 22, SWS_LOG_00006](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test
  `AP_R3_LOG_002_LoggerRetainsContextProperties`, using a valid unique context
  ID.
- Deviation: Application IDs, manifests, and cross-process registration are out
  of scope.

## AP-R3-LOG-003 - Logger creation and ownership

- Status: Approved
- Requirement: `CreateLogger()` shall create and return a logger owned by the
  logging framework. Application code shall not directly construct a logger.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §7.2.6, p. 22, SWS_LOG_00005;
  §8.2.1, p. 54, SWS_LOG_00021; §8.3.2, p. 68,
  SWS_LOG_00172](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_003_CreateLoggerOwnsLogger`.
- Deviation: Release 3 has no logger deregistration or platform lifecycle
  management.

## AP-R3-LOG-004 - Default console sink

- Status: Approved
- Requirement: When no generated logging model is available, a created logger
  shall use the console sink.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.2.1, pp. 54-55,
  SWS_LOG_00021 and SWS_LOG_00263](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_004_DefaultSinkIsConsole`.
- Deviation: Console is the only supported sink. DLT, file, and remote sinks are
  out of scope.

## AP-R3-LOG-005 - Logging operation

- Status: Approved
- Requirement: A logger shall support a log operation that accepts a severity
  level and message. A successful call shall emit one newline-terminated record.
- AUTOSAR source:
  [Specification of Log and Trace, R23-11, §8.3.2, pp. 68-72,
  SWS_LOG_00064 to SWS_LOG_00069 and
  SWS_LOG_00131](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_005_OneRecordPerLogCall`.
- Deviation: AdaptivePi uses one direct message API. `LogStream`, stream
  insertion, modeled messages, `LogWith`, and trace artifacts are out of scope.

## AP-R3-LOG-006 - Deterministic record format

- Status: Approved
- Requirement: The console sink shall format each record exactly as:

  `[LEVEL][CONTEXT] message\n`

- AUTOSAR source: No direct AUTOSAR requirement specifies this exact text
  format. It is an AdaptivePi decision inspired by AUTOSAR log severity and
  context concepts:
  [Specification of Log and Trace, R23-11, §7.2.4, p. 20;
  §8.1.1, p. 46](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf).
- Verification: Unit test `AP_R3_LOG_006_FormatsDeterministicRecord`.
- Deviation: This format is intentionally simpler than an AUTOSAR logging
  backend record.

## AP-R3-LOG-007 - Deterministic output

- Status: Approved
- Requirement: The console sink shall not add timestamps, process IDs, thread
  IDs, colour codes, or other environment-dependent data.
- AUTOSAR source: No direct AUTOSAR requirement. This is an AdaptivePi
  deterministic-testability decision.
- Verification: Unit test `AP_R3_LOG_007_OutputIsDeterministic`.
- Deviation: AUTOSAR logging backends may emit richer metadata. Release 3
  intentionally does not.

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
- Deviation: The failing sink exists only to verify this requirement. Production
  recovery, persistence, and diagnostics are out of scope.

## AP-R3-LOG-009 - Sequential ordering

- Status: Approved
- Requirement: Sequential logging calls on one logger shall reach its sink in
  call order.
- AUTOSAR source: No direct AUTOSAR requirement. This is an AdaptivePi
  deterministic-testability decision.
- Verification: Unit test `AP_R3_LOG_009_SequentialOutputOrder`.
- Deviation: Thread safety and ordering between multiple threads are out of
  scope.