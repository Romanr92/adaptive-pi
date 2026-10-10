# ara::log logger foundation

Release 3 provides a project-owned educational logging foundation, not a claim
of AUTOSAR API compatibility. Its baseline is
[LOG-001 through LOG-010](../requirements/release-3-ara-log.md); the accepted
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
Local exception-disabled checks passed; exception-enabled CI and ARM64
verification remain pending for these changes.

Allocation-failure injection uses a separate test executable with 64-bit Linux
GCC/Clang ABI symbols and ELF linker wrapping. CMake checks linker support and
omits that executable on unsupported hosts, including AppleClang/Mach-O, with
an explicit configure message. Portable foundation tests remain enabled; an
omitted injection target is not passing failure-path evidence. Linux CI supplies
that verification in both exception modes.
