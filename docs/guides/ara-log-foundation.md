# ara::log logger foundation

Release 3 provides a project-owned educational logging foundation, not a claim
of AUTOSAR API compatibility. Its baseline is
[LOG-001 through LOG-011](../requirements/release-3-ara-log.md); the accepted
factory deviation is explained in [ADR 0007](../adr/0007-release-3-logging-contracts.md).

## Create once, then borrow

Include `ara/log/logger.h` and link the CMake target `adaptive_pi_ara_log`.
Create a process-unique context during application initialization:

```cpp
auto result = ara::log::TryCreateLogger(
    "MAIN", "Application startup", ara::log::LogLevel::kInfo);

if (!result.HasValue())
{
  // Initialization decides whether operating without logging is acceptable.
  // Handle result.Error() using the application's error policy.
}
else
{
  auto& logger = result.Value().get();
  // Borrow logger; destroying result does not destroy the registered object.
  (void)logger;
}
```

TryCreateLogger copies the supplied ID and description, stores an immutable
threshold, and returns Result<reference_wrapper<Logger>, ErrorCode>. Resource
failure reports AdaptivePiErrc::kOperationFailed without a successful fallback
or partial registration. A defined threshold is required; an undefined enum
value terminates. Duplicate registration IDs violate the caller's precondition.

After successful registration, CreateLogger retrieves that same context:

```cpp
auto& logger = ara::log::CreateLogger(
    "MAIN", "Application startup", ara::log::LogLevel::kInfo);
```

This second operation is optional: the reference from the successful Result is
already usable. CreateLogger performs no context creation; missing registration
or different description/threshold terminates. This differs deliberately from
the AUTOSAR reference-returning creation contract.

Both operations support concurrent use. Framework ownership keeps addresses
stable as more loggers are registered. Applications cannot construct, copy,
move, or destroy loggers. References and accessor string views remain valid
until framework shutdown; all users must stop before shutdown. Logging from
static destructors is unsupported.

## Threshold and destination

IsEnabled admits defined severities from Fatal through the threshold. Off
severity, an Off threshold, and undefined severity values are disabled.
Stream submission applies this threshold before acquiring metadata or writing.

Each logger receives the registry-owned console sink. Its interface and
implementation remain under src/, outside the public include directory.
The console backend submits bounded bytes to buffered stdout and reports short
writes internally. Records include a UTC millisecond timestamp, Linux PID/TID,
severity, context ID, and escaped message, ending in one physical newline.

## Stream logging

Include `ara/log/logger.h` when using the stream API; it includes the complete
`LogStream` definition. After successfully registering a logger:

```cpp
auto& logger = result.Value().get();
logger.LogInfo() << "ready=" << true;

auto stream = logger.LogWarn();
stream << "retry=" << 1;
stream.Flush();
stream << "retry=" << 2;
```

Temporary destruction submits the first record. `Flush()` submits and clears
pending bytes; subsequent insertion starts a fresh record submitted on
destruction. Empty or filtered records emit nothing. Streams are move-constructible
but cannot be copied or move-assigned; moving transfers pending-record ownership.

Supported values are booleans, characters, integral types excluding wide
characters, float/double, non-null null-terminated character strings,
`std::string`, and `std::string_view`. Numbers use locale-independent formatting;
floating values use general format with `max_digits10` precision. Unsupported
types are rejected at compilation. The message limit is 4096 raw bytes; overflow
or internal metadata, formatting, or sink failure discards the affected record.
Separate streams may share a logger concurrently. Successful console writes
preserve complete records; thread ordering is unspecified.

## Modelled messages

A modelled message defines its identifier, severity, parameter types, and
renderer in manually authored C++. Release 3 does not generate definitions
from ARXML.

```cpp
#include "ara/log/logger.h"

#include <cstdint>
#include <string_view>
#include <tuple>

struct RetryMessage
{
  static constexpr std::string_view kId{"RETRY"};
  static constexpr ara::log::LogLevel kLevel{
      ara::log::LogLevel::kWarn};

  using ParameterTypes = std::tuple<std::uint32_t>;

  static void Render(ara::log::MessageWriter& writer,
                     const std::uint32_t& attempt) noexcept
  {
    constexpr std::string_view c_attempt_label{"attempt="};
    writer << c_attempt_label << attempt;
  }
};
```

After successfully registering a logger with a threshold admitting Warn:

```cpp
logger.Log(RetryMessage{}, std::uint32_t{2});
```

The message payload is:

```text
id=RETRY attempt=2
```

Message IDs must be non-empty ASCII letters, digits, or underscores.
`kLevel` has type `LogLevel`. `ParameterTypes` is a tuple declaring parameter
types in call order. Arguments must match after removing references and
top-level cv qualifiers; implicit numeric and string conversions are rejected.

Type matching and argument access are separate concerns. Volatile scalar
arguments are read into ordinary values for renderer and trace dispatch;
non-volatile arguments retain their original references. Each enabled
destination reads its volatile scalar arguments separately, so callers must
not assume both destinations observe one shared snapshot.

Removing qualifiers for matching does not provide volatile-qualified member
operations for class types. In particular, `std::string` and `std::string_view`
have no volatile-qualified accessors. The documented renderers taking ordinary
const references cannot consume volatile instances of those classes. Supply
ordinary string objects, or design a message renderer and trace specialization
for a project-owned type with an explicitly supported volatile access protocol.
Do not cast away volatile to call ordinary string methods.

`Render()` returns `void` and must be `noexcept`. It receives a borrowed
`MessageWriter` supporting the same insertion types and formatting as stream
logging. The renderer must finish synchronously and must not retain the writer.
The writer provides no flush operation; Logger controls record submission.

The payload starts with `id=<message-id>`. One space precedes non-empty
rendered text. A renderer producing no text emits only the ID and any
attributes. Context threshold filtering applies before rendering.

### Message attributes

`LogWith()` accepts a tuple containing at most one `Location` and one `Tag`.
Unsupported attribute types and duplicate types fail compilation.

```cpp
const auto attributes = std::make_tuple(
    ara::log::Location{"startup.cpp", 42},
    ara::log::Tag{"initialization"});

logger.LogWith(attributes, RetryMessage{}, std::uint32_t{2});
```

The payload before console escaping is:

```text
id=RETRY attempt=2 location="startup.cpp:42" tag="initialization"
```

Attributes appear in tuple order. Location file identifiers and Tag values
are borrowed string views; their backing storage must remain valid throughout
the call.

Attribute values escape backslashes and double quotes first. Console
rendering then applies the existing console escaping rules to the complete
payload.

The ID, rendered text, separators, and attributes share the 4096-byte message
limit before console escaping. Attribute escaping counts toward that limit.
Overflow or internal logging failure discards the entire record without
returning an application error. These rules apply in both exception modes.

## Compile-time trace routing

`TraceRouting<Message>` selects destinations for each modelled message type.
Its default enables logger output and disables tracing, preserving ordinary
modelled logging. Specialize it to select either destination independently:

| `kLoggerEnabled` | `kTraceEnabled` | Destination |
|---|---|---|
| `true` | `false` | Logger only (default) |
| `false` | `true` | Trace artifact only |
| `true` | `true` | Both |
| `false` | `false` | Discard |

For the `RetryMessage` above, this example enables both routes and counts trace
invocations through a manually authored educational artifact:

```cpp
#include <atomic>

namespace retry_trace
{
  inline std::atomic<std::uint32_t> calls{0};
}

namespace ara::log
{
  template <>
  struct TraceRouting<RetryMessage>
  {
    static constexpr bool kLoggerEnabled = true;
    static constexpr bool kTraceEnabled = true;
  };
}

namespace ara::log::ext
{
  template <>
  inline void TraceArti<RetryMessage, std::uint32_t>(
      const RetryMessage&, const std::uint32_t&) noexcept
  {
    retry_trace::calls.fetch_add(1, std::memory_order_relaxed);
  }
}
```

Place routing and trace specialization declarations in a shared header after
the message definition and before any call to `Log()` or `LogWith()` using that
message. All translation units must see consistent definitions. An inline
specialization can be defined in that header as above; alternatively, declare
it there and define it once in an application source file.

The primary `TraceArti(const MsgId&, const Params&...) noexcept` template has
no default definition. Every trace-enabled message and parameter signature
needs a matching definition; a missing definition causes a link error.
Logger-only and discard routes require no trace definition.

Trace receives the original message synchronously. Non-volatile parameters
retain their original references; volatile scalar parameters are passed as
temporary values with their top-level qualifiers removed, selecting the
normalized trace specialization. References to those temporary values are valid
only during the callback and must not be retained. Any retained reference to an
ordinary parameter must also respect the caller-owned object's lifetime.
`LogWith()` attributes are applied to logger output and are not passed to the
trace interface.

Threshold filtering affects only the logger destination. Configured trace
invocations still occur with a `kOff` threshold, filtered severity, or after
logger assembly, formatting, metadata, or sink failure. Trace specializations
must honor `noexcept` in both exception configurations. A specialization that
lets an exception escape violates the contract and terminates; recovery from
that violation is outside Release 3.

Routing is fixed at compilation. ARXML generation, runtime routing changes,
DLT transport, and external trace-tool integration remain out of scope.

## Verification limits

Foundation tests cover level encoding, context retention and ownership,
threshold eligibility, factory ownership and lookup, reference stability,
concurrent registration, and injected allocation failures.
Console backend and public logger-path tests verify stdout output, formatting,
escaping, and filtering. Private registry injection supplies deterministic
metadata and failure status for submission, suppression, and recovery tests.
Concurrent tests exercise shared and separate loggers, flush/reuse, destruction,
and complete headers/payloads. No test-only friendship is present in Logger.

Common tests are included in both exception configurations. Default local runs
use exceptions disabled; exception-enabled execution is verified in CI.
Requirement statuses and passing CI evidence remain separate from test existence.

LOG-010 tests cover modelled payloads, supported rendering values, exact
parameter matching, attribute eligibility, tuple order, escaping, and size
limits. Compiler checks include successful controls and rejected public calls.
Local exception-disabled checks pass. PR #68's Host CI run for commit
`8347611` passed both host exception configurations and the existing target
build. The new routing edge cases and smoke-test CI step added afterward still
need CI evidence for their revision.

LOG-011 tests cover all four routes, original trace argument identity, logger
filtering (including trace-only routing at Off), assembly overflow, console
formatting failure, metadata failure, and sink failure. Exception-enabled CI
also exercises trace delivery after a contained metadata-provider exception.

The opt-in `ADAPTIVE_PI_BUILD_LOG_SMOKE` CMake option builds
`ara_log_target_smoke` without GoogleTest or host test discovery. It instantiates
`Log()` and `LogWith()`, both attributes, all four routes, and real trace
specializations. The ARM64 CI job enables it and verifies its ELF architecture;
it does not execute the target binary. A successful library build alone does
not verify application-instantiated template code. Local native execution
verifies the smoke test's trace counter. The smoke executable also compiled and
linked locally with the Yocto SDK, and its ELF header reports AArch64. Target
runtime execution remains unverified; cross-compilation is separate evidence.

Allocation-failure injection uses a separate test executable with 64-bit Linux
GCC/Clang ABI symbols and ELF linker wrapping. CMake checks linker support and
omits that executable on unsupported hosts, including AppleClang/Mach-O, with
an explicit configure message. Portable foundation tests remain enabled; an
omitted injection target is not passing failure-path evidence. Linux CI supplies
that verification in both exception modes.
