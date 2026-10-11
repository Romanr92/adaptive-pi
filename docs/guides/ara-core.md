# ara::core API guide

Release 3 implements the project-owned [CORE-001 through CORE-012
baseline](../requirements/release-3-ara-core.md). This is an educational,
clean-room subset inspired by AUTOSAR, without AUTOSAR conformance or full API
compatibility. Link `adaptive_pi_ara_core`; its public headers require C++17.

## Domains and error codes

`ErrorDomain` identifies an error family by a stable `std::uint64_t` ID and a
non-empty name. Identity comparison uses the ID alone. `ErrorCode` stores an
`std::int32_t` value and a borrowed domain reference: the domain and the backing
storage for its name must outlive every referring error code. Use static storage
for application domains. Construction, ID access, and equality support constant
expressions.

The supplied domain is `AdaptivePi`, ID `0x4150490000000001`.
`AdaptivePiErrc` defines `kInvalidArgument`, `kInvalidState`, and
`kOperationFailed`. `MakeErrorCode()` converts these values; equality and
inequality work with the enumeration in either argument order. Error-code
equality requires both the value and domain identity to match.

## Return and inspect a result

```cpp
#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/result.h"

ara::core::Result<int> ReadCount(bool available)
{
  if (!available)
  {
    return ara::core::Result<int>::FromError(
        ara::core::MakeErrorCode(ara::core::AdaptivePiErrc::kOperationFailed));
  }
  return ara::core::Result<int>::FromValue(42);
}

int main()
{
  const auto count = ReadCount(true);
  if (!count.HasValue())
  {
    return count.Error().Value();
  }
  return count.Value() == 42 ? 0 : 1;
}
```

`Result<T, E = ErrorCode>` holds one active value or error. `HasValue()` and
explicit `operator bool()` report the alternative, independently of payload
truthiness. `Result<void, E>::FromValue()` represents successful completion;
`Value()` then checks success without returning a payload.

Direct value construction accepts the value type; error construction is
explicit. Prefer `FromValue(...)` and `FromError(...)`, which also construct
payloads in place from constructor arguments. When `T` and `E` are identical,
direct construction selects the value; select an error with `FromError(...)`.
`EmplaceValue(...)` and `EmplaceError(...)` replace the active alternative.

`Value()` provides mutable, const, or rvalue access. `Error()` provides const
or rvalue access. Inspect state first: accessing the wrong alternative terminates
the process in both exception configurations. References into a result become
invalid when that payload is replaced or destroyed.

Copy and move construction depend on payload capabilities. Moving preserves
the destination's alternative. Do not inspect a moved-from result; only destroy
it or use an operation explicitly supported by its contract. Copy and move
assignment are currently deleted; use emplacement on an ordinary live result.
Payload destructors must not throw.

## Exception configuration

The CMake option `ADAPTIVE_PI_ENABLE_EXCEPTIONS` defaults to `OFF` and propagates
`-fno-exceptions` and `ADAPTIVE_PI_EXCEPTIONS_ENABLED=0` to consumers. With `ON`,
it propagates `-fexceptions` and the macro value `1`. Keep libraries and consumers
in the same mode; see [ADR 0006](../adr/0006-exception-build-policy.md).

`ValueOrThrow()` exists only in the enabled build for results whose error type
is `ErrorCode`, including void results. Success returns the value (or completes
for void); failure invokes the domain converter. `AdaptivePiException` exposes
`Error()` and a fixed `what()` message. A missing or returning converter violates
the conversion contract and terminates. Ordinary `Value()` still terminates on
an error; enabling exceptions does not change its contract.

For an additional project-owned domain, supply a unique ID, a domain-specific
enum and its conversion/comparison functions. With exceptions enabled, also
supply a domain-specific exception and converter when constructing the domain.
No changes to `ErrorCode` or `Result` are needed. The extensibility tests show
both configurations in [error_domain_extensibility_test.cpp](../../platform/ara-core/tests/error_domain_extensibility_test.cpp).

In the enabled build, emplacement constructs the replacement before releasing
the current alternative. If construction throws, the existing payload remains
active. Constructor side effects, including modifications through aliased
arguments, are not rolled back. Initial construction can propagate payload
exceptions. In the default build, perform fallible work through explicit
result-returning operations; disabling exceptions does not make all standard
library failures recoverable.

## Scope and verification

Release 3 excludes vendor support data, generated domains, `Future`, `Promise`,
public `Optional`, and unlisted core utilities. Internal use of standard-library
storage types does not add them to the public API. Domain definitions are
manually authored. There is no proprietary AUTOSAR implementation code.

See the component [design decisions](../../platform/ara-core/doc/design-decisions.md),
the [logging API guide](ara-log-foundation.md), and the
[Release 3 evidence record](../verification/release-3.md). Requirement statuses,
test declaration existence, and passing executions are separate evidence.
