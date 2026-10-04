# ara::log logger foundation

Release 3 provides a project-owned educational logging foundation, not a claim
of AUTOSAR API compatibility. Its baseline is
[LOG-001 through LOG-004](../requirements/release-3-ara-log.md); the accepted
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
Actual record-output filtering is implemented with stream logging in issue #15.

Each logger receives the registry-owned console sink. Its interface and
implementation remain under src/, outside the public include directory.
The console backend submits bounded bytes to buffered stdout and reports short
writes internally. Record formatting, metadata, stream operations, and logging
failure-discard behavior belong to issue #15.

## Verification limits

Foundation tests cover level encoding, context retention and ownership,
threshold eligibility, factory ownership and lookup, reference stability,
concurrent registration, and injected allocation failures.
Console backend tests verify exact stdout bytes. Factory sink wiring is verified
by source inspection; output through the public logger API is deferred to issue
#15. No test-only friendship is present in the production Logger definition.

Common tests are included in both exception configurations. Default local runs
use exceptions disabled; exception-enabled execution is verified in CI.
Requirement statuses and passing CI evidence remain separate from test existence.

Allocation-failure injection uses a separate test executable with 64-bit Linux
GCC/Clang ABI symbols and ELF linker wrapping. CMake checks linker support and
omits that executable on unsupported hosts, including AppleClang/Mach-O, with
an explicit configure message. Portable foundation tests remain enabled; an
omitted injection target is not passing failure-path evidence. Linux CI supplies
that verification in both exception modes.
