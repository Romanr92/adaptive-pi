# ADR 0007: Define Release 3 logging contracts

- Status: Accepted
- Date: 2026-10-04

## Context

LOG-001 through LOG-011 are allocated to issues #3, #15, and #16, but the
original baseline left lifetime, filtering, stream ownership, and several
observable formatting and failure behaviors unspecified. These choices affect
logger-foundation interfaces and must be settled before implementation.

This ADR records AdaptivePi decisions. It does not expand the claim of AUTOSAR
compatibility. The normative contracts and planned checks remain in
[the logging requirements](../requirements/release-3-ara-log.md).

## Decision

### Logger foundation — issue #3

CreateLogger returns a borrowed Logger reference. The framework owns immutable
copies of context inputs and keeps references stable until process shutdown.
Applications cannot construct, copy, move, or destroy loggers. Framework storage
must therefore keep object addresses stable as the registry grows. Registry
synchronization supports concurrent creation with caller-unique IDs; duplicate
IDs remain a caller precondition violation.

Creation-time resource failure is recoverable through the project-owned
TryCreateLogger factory returning Result<reference_wrapper<Logger>, ErrorCode>.
Application initialization decides whether logging is mandatory. Release 4 owns
supervisor recovery. The framework must leave its registry unchanged on failure.

CreateLogger retains a noexcept reference-returning shape but requires successful
prior registration with identical inputs. It retrieves that context without
fallible allocation. Violating this precondition terminates; resource exhaustion
in the recoverable factory does not. This two-operation protocol is a deliberate
AdaptivePi deviation, not the AUTOSAR creation contract. Static-destruction
logging remains unsupported.

The console sink writes to stdout. Its abstraction, explicit failure status,
and test injection facilities remain private. No public sink-selection API is
introduced.

### Streams and console records — issue #15

The creation threshold is immutable. Only defined severities from Fatal through
the threshold are emitted; Off suppresses output. Runtime threshold changes
remain outside Release 3. Threshold filtering also applies to modelled logger
output implemented by issue #16.

A stream exclusively owns its pending record: copying and move assignment are
unavailable, while move construction transfers responsibility for submission.
Empty streams do not emit. Flush clears pending state, including after failure,
and subsequent insertion starts another record. Each stream is used by one
thread at a time; different streams may use the same logger concurrently.

Console records use UTC timestamps with millisecond precision, numeric Linux
process/thread IDs, uppercase level names, and the context ID. Metadata is
sampled at submission. Escaping prevents embedded newlines from splitting a
record. Successful framework writes are serialized at whole-record granularity;
unrelated stdout writers and partial failed writes are outside this guarantee.

Supported value types and their rendering are enumerated in LOG-005. Formatting
must not depend on the process locale. No user-defined insertion callbacks are
required.

Message assembly uses a 4096-byte bound before console escaping. Overflow
invalidates the entire pending record; it does not silently truncate it.
Formatting must allow for escaped output expansion without throwing allocation.
This provides an explicit failure path in the default exception-disabled build.
A noexcept annotation alone would turn an escaping exception into termination,
which does not satisfy the discard contract.

Internal providers and sinks use explicit failure status. Enabled builds also
contain internal exceptions; disabled builds use non-throwing paths. No failure
logs recursively. Creation failure is deliberately separate from record failure.

### Modelled messages and routing — issue #16

Parameter validation uses exact types after removing references and top-level
cv qualifiers. Implicit conversion is not part of the educational API. Message
IDs and typed Location/Tag attributes supplied in a tuple have a deterministic payload format
specified in LOG-010. They share the stream backend's size and failure rules.

Threshold filtering applies only to the logger destination. A configured trace
invocation still occurs with Off or after logger failure. Trace specializations
must honor their noexcept contract; the framework cannot recover from a
specialization that violates it.

## Consequences and verification

These contracts keep runtime configuration, manifests, new production sinks,
and lifecycle management outside Release 3. Private failure injection and
metadata stubs provide deterministic tests without host resource exhaustion.

Issue #3 owns creation concurrency tests even though concurrent record emission
belongs to #15. Issue #15 owns threshold behavior tests; #16 owns trace/filter
interaction tests. The requirement inventory records additional planned test
names. Issue #5 collects evidence, including common tests in both exception
modes, enabled-only exception containment tests in CI, and ARM64 cross-build.
No new tests or production implementation are provided by this ADR.

## Official-source review and limits

Reviewed AUTOSAR AP R23-11 Log and Trace, §7.2.6 p. 22, §7.3.2.3–4
pp. 34, §8.2.1 p. 54, §8.3.1 pp. 57–59, and §8.3.2.9–10 p. 72.
Source: https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf

The specification requires framework ownership and a reentrant noexcept
reference-returning factory. It does not prescribe our recoverable factory or
an allocation-exhaustion recovery mechanism. This ADR supersedes its original
unconditional creation-resource-failure termination decision.

LogWith uses a tuple of attributes, with Location and Tag described in §7.3.2.4.
The original arbitrary key/value design is replaced. Exact normalized parameter
matching and console rendering remain explicit project choices; the source
requires mismatched modelled arguments to be ill-formed without establishing
our precise matching algorithm. Move-only stream rules are project choices,
not verified AUTOSAR special-member restrictions. Flush starts a new stream,
but the source note says it does not empty the buffer. Our fresh-record,
no-duplicate contract deliberately resolves that ambiguity for this backend.
