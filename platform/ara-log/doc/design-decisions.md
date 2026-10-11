# ara::log design decisions

Decision IDs are local to this component and remain stable. Code annotations use
`/* Implemented DD-01 */`; resolve each ID against this component document.
The approved [requirement baseline](../../../docs/requirements/release-3-ara-log.md)
defines behavior. This is a clean-room educational subset, without AUTOSAR
conformance or an expansion of release scope.

## DD-01 — Framework ownership and stable context references

Requirements: LOG-002, LOG-003.

The process-local registry owns separately allocated loggers with copied immutable context strings and thresholds. Registration never relocates existing objects. Applications cannot construct, copy, move, or destroy loggers. IDs must be caller-unique; users stop before framework shutdown. Static-destructor logging is unsupported.

Implementation: [logger_registry.h](../src/logger_registry.h), [logger.h](../include/ara/log/logger.h).

## DD-02 — Mutex-protected registration and lookup

Requirements: LOG-003.

A POSIX mutex protects publication and lookup of the registry list and reports locking failures explicitly in both exception configurations. The registry is process-local; manifests and cross-process registration are excluded.

Implementation: [logger_registry.h](../src/logger_registry.h).

## DD-03 — Recoverable registration before borrowed lookup

Requirements: LOG-003.

TryCreateLogger uses non-throwing allocation and publishes only after successful construction and locking. Failure returns Result with kOperationFailed without partial registration. CreateLogger retrieves an already registered context with matching inputs; invalid lookup terminates. This two-operation protocol deliberately differs from AUTOSAR creation semantics.

Implementation: [logger_registry.cpp](../src/logger_registry.cpp), [ara_log.cpp](../src/ara_log.cpp).

## DD-04 — Private console sink and Linux metadata dependencies

Requirements: LOG-004, LOG-007.

The registry supplies its owned stdout console sink and Linux metadata provider. Dependency injection remains private for deterministic verification. No public sink selection, deregistration, or runtime threshold API is introduced.

Implementation: [logger_registry.h](../src/logger_registry.h).

## DD-05 — Move-only stream submission and reuse

Requirements: LOG-005.

A stream exclusively owns pending bytes. Move construction transfers submission responsibility; copying and move assignment are unavailable. Flush or destruction submits once; flush clears state even after failure and reuse starts a new record. Empty streams emit nothing. Each stream has one thread using it at a time; separate streams may share a logger.

Implementation: [log_stream.h](../include/ara/log/log_stream.h), [log_stream.cpp](../src/log_stream.cpp).

## DD-06 — Filter before metadata acquisition

Requirements: LOG-002, LOG-005.

Immutable creation thresholds admit only defined severities from Fatal through the threshold. Off suppresses logger output. Submission checks filtering before metadata acquisition and writing, avoiding work and failures for suppressed records.

Implementation: [logger.cpp](../src/logger.cpp), [logger.cpp](../src/logger.cpp).

## DD-07 — Bounded assembly and complete-record discard

Requirements: LOG-005, LOG-006, LOG-008.

Messages have a 4096-byte raw bound; rendered console records use a private 16384-byte fixed buffer. Checked appends avoid allocation during formatting. Escaping expands data and context length is not publicly bounded, so a valid raw message can exceed rendered capacity. Overflow discards the complete record without truncation. Both capacities are AdaptivePi decisions.

Implementation: [log_stream.h](../include/ara/log/log_stream.h), [record_buffer.h](../src/record_buffer.h).

## DD-08 — Project-defined console format and insertion subset

Requirements: LOG-005, LOG-006, LOG-007.

Records use UTC milliseconds, Linux PID/TID, uppercase severity, context ID, escaped payload, and one final newline. Metadata is sampled at submission. Supported insertion values and locale-independent numeric rendering form a focused subset. This text representation and no-replay flush contract do not claim AUTOSAR backend equivalence.

Implementation: [console_record_formatter.cpp](../src/console_record_formatter.cpp), [log_stream.cpp](../src/log_stream.cpp).

## DD-09 — Discard logging failures through explicit status

Requirements: LOG-008.

Assembly, metadata, formatting, and sink failures discard the affected record without an application error or recursive failure logging. Enabled builds contain provider exceptions at submission. Sinks must honor noexcept and return failure status; an escaping exception terminates before caller recovery. Creation failure is handled separately by DD-03.

Implementation: [logger.cpp](../src/logger.cpp).

## DD-10 — Successful whole-record stdout writes

Requirements: LOG-009.

One fwrite call submits the complete record under the console stream synchronization. Successful writes do not interleave framework records; ordering between threads is unspecified. Unrelated stdout writers and failed partial writes are outside this guarantee.

Implementation: [console_sink.cpp](../src/console_sink.cpp).

## DD-11 — Compile-time modelled contracts and synchronous rendering

Requirements: LOG-010.

Public templates validate exact normalized argument types, valid message IDs, and unique Location/Tag attributes. A noexcept renderer consumes a borrowed writer synchronously. ID, text, and attributes share the raw bound; attribute escaping precedes console escaping. Definitions are manually authored rather than generated from ARXML.

Implementation: [logger.h](../include/ara/log/logger.h).

## DD-12 — Independent compile-time trace destinations

Requirements: LOG-011.

Manually authored TraceRouting specializations select logger, trace, both, or discard. Logger filtering and failure never suppress a configured trace call. TraceArti definitions must exist and honor the synchronous noexcept contract; missing definitions fail at link time. Translation units must see consistent specializations before use. This educational boundary excludes runtime routing, DLT, and production trace tools.

Implementation: [trace_routing.h](../include/ara/log/trace_routing.h), [trace_artifact.h](../include/ara/log/trace_artifact.h), [logger.h](../include/ara/log/logger.h).

## Verification

Requirement tests, compile-time contracts, failure injection, and target smoke
checks verify these decisions. Common behavior runs in both exception modes;
guarded throwing paths run in enabled CI. Test injection remains private and
Linux ELF allocation wrapping remains confined to test executables. See the
[Release 3 evidence](../../../docs/verification/release-3.md) for execution
evidence and the [API guide](../../../docs/guides/ara-log-foundation.md) for supported interfaces and limits.
