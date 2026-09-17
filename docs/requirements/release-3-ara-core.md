# Release 3 - clean-room ara::core-inspired requirements

## Scope

Release 3 provides a project-owned error domain, error code, and `Result`
template for returning either a value or a recoverable error from synchronous
operations.

The implementation shall favour the exception-free error-handling style used by
AUTOSAR Adaptive Platform interfaces.

## AP-R3-CORE-001 - ErrorDomain

- Status: Draft
- Requirement: The component shall provide an `ErrorDomain` type that defines
  the context for a set of related error conditions.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.2, p. 27,
  SWS_CORE_10303](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- Verification: Unit test
  `AP_R3_CORE_001_ErrorDomainProvidesErrorContext`.
- Deviation: Release 3 provides one project-owned error domain only. ARXML
  generation and AUTOSAR-defined domains are out of scope.

## AP-R3-CORE-002 - ErrorDomain identity

- Status: Draft
- Requirement: Every `ErrorDomain` shall have a stable, unique
  `std::uint64_t` identifier and a non-empty name. Two error domains shall
  compare equal if and only if their identifiers are equal.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.2, p. 27,
  SWS_CORE_10401; §8.1.1, pp. 49-52, SWS_CORE_00121, SWS_CORE_00151,
  SWS_CORE_00152](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- Verification: Unit test
  `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName`.
- Deviation: Literal-type and `constexpr` requirements are out of scope.

## AP-R3-CORE-003 - ErrorCode contents

- Status: Draft
- Requirement: `ErrorCode` shall contain an integral error value and a
  reference to its originating `ErrorDomain`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.1, p. 26,
  SWS_CORE_10302; §8.1.2, pp. 54-55, SWS_CORE_00514,
  SWS_CORE_00515](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.1, p. 8](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test
  `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain`.
- Deviation: Vendor support data is out of scope.

## AP-R3-CORE-004 - ErrorCode equality

- Status: Draft
- Requirement: Two `ErrorCode` objects shall compare equal if and only if both
  their error values and originating error domains are equal.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.1, p. 26,
  SWS_CORE_10301](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit test `AP_R3_CORE_004_ErrorCodeEquality`.
- Deviation: Comparison with domain-specific enumerations is out of scope.

## AP-R3-CORE-005 - Result states

- Status: Draft
- Requirement: `Result<T, E = ErrorCode>` shall represent exactly one active
  alternative: a value of type `T` or an error of type `E`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.3, p. 29,
  SWS_CORE_10600; §8.1.4, p. 59,
  SWS_CORE_00701](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, p. 8](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test `AP_R3_CORE_005_ResultHasExactlyOneState`.
- Deviation: `Result<void, E>` is out of scope.

## AP-R3-CORE-006 - Result creation

- Status: Draft
- Requirement: `Result<T, E>` shall provide the static factory functions
  `FromValue(...)` and `FromError(...)`. `FromValue(...)` shall create a value
  result and `FromError(...)` shall create an error result.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 62-64,
  SWS_CORE_00731 to SWS_CORE_00736](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2.1, p. 9](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test `AP_R3_CORE_006_ResultCreation`.
- Deviation: Release 3 does not require implicit conversion constructors,
  emplacement overloads, or direct construction from a value or error.

## AP-R3-CORE-007 - Result state query

- Status: Draft
- Requirement: `HasValue()` shall return `true` if and only if the result
  contains a value.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 65-66,
  SWS_CORE_00751](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, pp. 9-10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test `AP_R3_CORE_007_HasValueReportsState`.
- Deviation: `operator bool()` is out of scope.

## AP-R3-CORE-008 - Value access

- Status: Draft
- Requirement: `Value()` shall provide access to the stored value only when
  `HasValue()` is `true`. Calling `Value()` on an error result shall be treated
  as a violation and shall terminate the process.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 67-68,
  SWS_CORE_00755 to SWS_CORE_00756; §7.2.1.3, p. 25,
  SWS_CORE_00003](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, p. 10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Death test `AP_R3_CORE_008_ValueOnErrorTerminates`.
- Deviation: AdaptivePi terminates the process and does not provide
  `ValueOrThrow()` or AUTOSAR exception conversion.

## AP-R3-CORE-009 - Error access

- Status: Draft
- Requirement: `Error()` shall provide access to the stored error only when
  `HasValue()` is `false`. Calling `Error()` on a value result shall be treated
  as a violation and shall terminate the process.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 68-69,
  SWS_CORE_00757 to SWS_CORE_00758; §7.2.1.3, p. 25,
  SWS_CORE_00003](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, p. 10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Death test `AP_R3_CORE_009_ErrorOnValueTerminates`.
- Deviation: AdaptivePi terminates the process and does not provide
  `ValueOrThrow()` or AUTOSAR exception conversion.

## AP-R3-CORE-010 - Result move state

- Status: Draft
- Requirement: Moving a `Result` shall preserve its active alternative in the
  destination object.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, p. 61,
  SWS_CORE_00726; p. 64,
  SWS_CORE_00742](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit test `AP_R3_CORE_010_MovePreservesState`.
- Deviation: The moved-from state is unspecified and is not tested.

## AP-R3-CORE-011 - Exception-free Result use

- Status: Draft
- Requirement: Release 3 public operations that can report recoverable failures
  shall use `Result` rather than C++ exceptions.
- AUTOSAR source:
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1, p. 8;
  §3.1.2, pp. 8-10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.3, p. 25,
  SWS_CORE_00002](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit tests for every Release 3 operation that has a recoverable
  failure path.
- Deviation: `ara::log` follows its specific AUTOSAR-inspired policy in
  AP-R3-LOG-008: internal logging failures are discarded rather than reported.