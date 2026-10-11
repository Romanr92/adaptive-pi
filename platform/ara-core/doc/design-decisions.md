# ara::core design decisions

Decision IDs are local to this component and remain stable. Code annotations use
`/* Implemented DD-01 */`; resolve each ID against this component document.
The approved [requirement baseline](../../../docs/requirements/release-3-ara-core.md)
defines behavior. This is a clean-room educational subset, without AUTOSAR
conformance or an expansion of release scope.

## DD-01 — Stable domain identity

Requirements: CORE-001, CORE-002, CORE-012.

`ErrorDomain` is a concrete literal type with a stable 64-bit ID and a non-empty borrowed name. Equality compares IDs alone; constexpr construction and access support compile-time checks. Additional project-owned domains require unique IDs. This follows the AUTOSAR-inspired baseline without promising ABI compatibility.

Implementation: [error_domain.h](../include/ara/core/error_domain.h).

## DD-02 — Borrowed domain lifetime and fixed error value

Requirements: CORE-003, CORE-004.

`ErrorCode` stores a 32-bit value and borrows its domain. The domain and its name storage must outlive referring codes. Equality compares value and domain identity. Borrowing avoids copying domain definitions and permits extensibility without changing the container.

Implementation: [error_code.h](../include/ara/core/error_code.h).

## DD-03 — One manually authored concrete domain

Requirements: CORE-004, CORE-012.

The supplied AdaptivePi domain uses inline constexpr static storage, a published stable ID, and a domain-specific enumeration with conversion and comparison functions. Generated domains and vendor support data remain out of scope. Further domains own their enums and conversions.

Implementation: [adaptive_pi_error_domain.h](../include/ara/core/adaptive_pi_error_domain.h).

## DD-04 — Two-slot replacement with non-throwing destruction

Requirements: CORE-005, CORE-006.

`ResultStorage` keeps two optional variant slots and one active-slot index. Replacement constructs into the inactive slot before switching and destroying the previous payload. Additional storage preserves the active payload if enabled-build construction throws, without a throwing move to commit it. Payload destructors must not throw. Constructor side effects through aliased arguments are not rolled back. This storage strategy is an AdaptivePi choice.

Implementation: [result.h](../include/ara/core/result.h).

## DD-05 — Explicit alternatives and recoverable results

Requirements: CORE-005, CORE-006, CORE-011.

`Result<T, E = ErrorCode>` represents one value or error and is the boundary for recoverable public failures. Factories select an alternative explicitly and support in-place arguments. When T and E are identical, direct construction selects the value; FromError selects the error. Result itself does not make all payload failures recoverable.

Implementation: [result.h](../include/ara/core/result.h).

## DD-06 — Checked state and access

Requirements: CORE-007, CORE-008, CORE-009.

Queries and access validate active storage. Wrong-alternative access and internal invariant violations terminate in both exception modes. HasValue and explicit bool depend on the alternative, not payload truthiness. These checks prevent use of inactive payloads.

Implementation: [result.h](../include/ara/core/result.h), [result.h](../include/ara/core/result.h).

## DD-07 — Construction and emplacement without assignment

Requirements: CORE-006, CORE-010.

Copy/move construction follows payload capabilities and preserves the destination alternative. Assignment is deleted; emplacement replaces payloads on an ordinary live result. The moved-from object must not be inspected. This is a reduced educational API, not full AUTOSAR API compatibility.

Implementation: [result.h](../include/ara/core/result.h).

## DD-08 — Void success uses an internal empty payload

Requirements: CORE-005, CORE-006.

The void specialization represents successful completion using std::monostate internally, or an error. FromValue creates success; Value checks success without returning a payload. Internal standard-library storage does not add a public Optional API.

Implementation: [result.h](../include/ara/core/result.h).

## DD-09 — Consistent optional exception conversion

Requirements: CORE-008, CORE-011.

The default CMake mode disables exceptions. All consumers share compiler flags and configuration definitions; config.h rejects disagreement with compiler support. Disabled builds exclude throwing converters and ValueOrThrow declarations. Enabled builds allow explicit domain conversion only for ErrorCode results. A missing or returning converter terminates. ADR 0006 owns the project-wide build policy.

Implementation: [config.h](../include/ara/core/config.h), [error_domain.h](../include/ara/core/error_domain.h), [result.h](../include/ara/core/result.h).

## Verification

Requirement tests, compile-time contracts, failure injection, and target smoke
checks verify these decisions. Common behavior runs in both exception modes;
guarded throwing paths run in enabled CI. Test injection remains private and
Linux ELF allocation wrapping remains confined to test executables. See the
[Release 3 evidence](../../../docs/verification/release-3.md) for execution
evidence and the [API guide](../../../docs/guides/ara-core.md) for supported interfaces and limits.
