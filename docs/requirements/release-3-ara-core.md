# Release 3 - clean-room ara::core-inspired requirements

## Scope

Release 3 provides a project-owned error domain, error code, and `Result`
template for returning either a value or a recoverable error from synchronous
operations.

The implementation shall support both exception-disabled and exception-enabled
builds. Repository builds shall disable exceptions by default using
`-fno-exceptions` on GCC/Clang. An explicit build option shall enable exceptions
without source edits. This default is an AdaptivePi decision, not an AUTOSAR
requirement; see [ADR 0006](../adr/0006-exception-build-policy.md).

Both configurations shall support exception-free handling through `Result`.
Exception-based retrieval through `ValueOrThrow()` shall be available only when
C++ exceptions are enabled in the compiler toolchain.


## Verification evidence

Verification entries name the planned checks. Checkmarks record declaration
existence only, not test execution or complete coverage. Exception-disabled core
execution has been observed locally; exception-enabled verification is delegated
to Host CI and remains unconfirmed here without a successful run for the current
revision. Existing Status Deviation entries explicitly retain their accepted
status convention and do not claim both configurations passed.

The CORE-006 convertible-error constructor defect is fixed. Its regression test
and all 154 core tests passed locally with exceptions disabled. CORE-006 remains
Approved pending confirmed exception-enabled CI verification.

## AP-R3-CORE-001 - ErrorDomain

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: The component shall provide an `ErrorDomain` type that defines
  the context for a set of related error conditions.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.2, p. 27,
  SWS_CORE_10303](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- Verification: Unit test
  `AP_R3_CORE_001_ErrorDomainProvidesErrorContext` ✅.
- Unit Tests: `AP_R3_CORE_001_ErrorDomainProvidesErrorContext.ReturnsConfiguredIdentifier`; `AP_R3_CORE_001_ErrorDomainProvidesErrorContext_ReturnsConfiguredName.ReturnsConfiguredName`
- Deviation: Release 3 delivers one concrete project-owned error domain.
  `ErrorDomain` and `ErrorCode` shall support additional project-owned error
  domains with unique identifiers without changes to `Result` or `ErrorCode`.
  ARXML generation and the AUTOSAR-defined set of concrete error domains are
  out of scope.

## AP-R3-CORE-002 - ErrorDomain identity and compile-time use

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: Every `ErrorDomain` shall have a stable, unique
  `std::uint64_t` identifier and a non-empty name. Two error domains shall
  compare equal if and only if their identifiers are equal.
- Requirement: Every `ErrorDomain` shall be a literal type and shall be
  `constexpr` constructible. Its identifier accessor and equality comparison
  shall be usable in constant expressions.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.2, pp. 27-28,
  SWS_CORE_10400 and SWS_CORE_10401; §8.1.1, pp. 49-51,
  SWS_CORE_00121, SWS_CORE_00135, SWS_CORE_00137, SWS_CORE_00151, and
  SWS_CORE_00152](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- Verification: Compile-time verification using `static_assert`, plus unit test
  `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName` ✅.
- Unit Tests: `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName.PreservesConfiguredIdentifier`; `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_ProvidesNonEmptyName.ProvidesNonEmptyName`; `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_SameIdentifiersCompareEqual.SameIdentifiersCompareEqual`; `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_DifferentIdentifiersCompareUnequal.DifferentIdentifiersCompareUnequal`; `AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_EmptyNameTerminates.EmptyNameTerminates`
- Deviation: The Release 3 concrete domain is project-owned rather than
  ARXML-generated.

## AP-R3-CORE-003 - ErrorCode contents

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `ErrorCode` shall contain an integral error value and a
  reference to its originating `ErrorDomain`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.1, p. 26,
  SWS_CORE_10302; §8.1.2, pp. 54-55, SWS_CORE_00514 and
  SWS_CORE_00515](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.1, p. 8](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test
  `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain` ✅.
- Unit Tests: `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain.StoresIntegralErrorValue`; `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain_ReferencesExactOriginatingDomain.ReferencesExactOriginatingDomain`
- Deviation: Vendor support data is out of scope.

## AP-R3-CORE-004 - ErrorCode comparison

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: Two `ErrorCode` objects shall compare equal if and only if both
  their error values and originating error domains are equal.
- Requirement: Every concrete `ErrorDomain` shall define a domain-specific
  error enumeration and provide conversion from that enumeration to
  `ErrorCode`. `ErrorCode` shall support equality and inequality comparison
  with the corresponding domain-specific error enumeration.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.1, p. 26,
  SWS_CORE_10301; §7.2.1.7.5, p. 34, SWS_CORE_10990 and
  SWS_CORE_10991; §8.1.2.1, pp. 56-57, SWS_CORE_00571 and
  SWS_CORE_00572](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit test
  `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison` ✅.
- Unit Tests: `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison.EnumConversionStoresValueAndDomain`; `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_EquivalentErrorCodesCompareEqual.EquivalentErrorCodesCompareEqual`; `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentValuesInSameDomainCompareUnequal.DifferentValuesInSameDomainCompareUnequal`; `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_SameValueInDifferentDomainsCompareUnequal.SameValueInDifferentDomainsCompareUnequal`; `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_MatchingEnumComparesEqualInBothDirections.MatchingEnumComparesEqualInBothDirections`; `AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentEnumComparesUnequalInBothDirections.DifferentEnumComparesUnequalInBothDirections`
- Deviation: None.

## AP-R3-CORE-005 - Result states

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `Result<T, E = ErrorCode>` shall represent exactly one active
  alternative: a value of type `T` or an error of type `E`.
- Requirement: `Result<void, E>` shall represent exactly one active
  alternative: successful completion without a value or an error of type `E`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.3, p. 29,
  SWS_CORE_10600; §8.1.4, p. 59, SWS_CORE_00701; §8.1.4.1,
  pp. 75-82](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, p. 8](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit tests
  `AP_R3_CORE_005_ResultHasExactlyOneState` ✅ and
  `AP_R3_CORE_005_ResultVoidHasExactlyOneState` ✅.
- Unit Tests: `AP_R3_CORE_005_ResultHasExactlyOneState.ReportsSelectedState`; `AP_R3_CORE_005_ResultHasExactlyOneState_CopyConstructionPreservesState.CopyConstructionPreservesState`; `AP_R3_CORE_005_ResultHasExactlyOneState_MoveConstructionPreservesState.MoveConstructionPreservesState`; `AP_R3_CORE_005_ResultHasExactlyOneState_SameTypesAndPayloadsKeepAlternativesDistinct.SameTypesAndPayloadsKeepAlternativesDistinct`; `AP_R3_CORE_005_ResultHasExactlyOneState_ConstructsAndDestroysOnlySelectedAlternative.ConstructsAndDestroysOnlySelectedAlternative`; `AP_R3_CORE_005_ResultVoidHasExactlyOneState.ReportsSelectedState`; `AP_R3_CORE_005_ResultVoidHasExactlyOneState_CopyConstructionPreservesState.CopyConstructionPreservesState`; `AP_R3_CORE_005_ResultVoidHasExactlyOneState_MoveConstructionPreservesState.MoveConstructionPreservesState`; `AP_R3_CORE_005_ResultVoidHasExactlyOneState_ConstructsAnErrorOnlyOnFailure.ConstructsAnErrorOnlyOnFailure`; `AP_R3_CORE_006_ReplacementTransitions.PreservesPayloadOnFailureAndReplacesOnSuccess`; `AP_R3_CORE_006_VoidReplacement.PreservesStateOnFailureAndReplacesOnSuccess`
- Deviation: None.

## AP-R3-CORE-006 - Result creation

- Status: Approved
- Requirement: `Result<T, E>` shall support construction from a value of type
  `T` and explicit construction from an error of type `E`.
- Requirement: `Result<T, E>` shall provide the static factory functions
  `FromValue(...)` and `FromError(...)`. These factories shall support
  in-place construction from constructor arguments.
- Requirement: `Result<T, E>` shall provide `EmplaceValue(...)` and
  `EmplaceError(...)` to replace its active alternative with an object
  constructed in place.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 60-65,
  SWS_CORE_00721 to SWS_CORE_00724, SWS_CORE_00731 to SWS_CORE_00736,
  SWS_CORE_00743, and SWS_CORE_00744](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2.1, p. 9](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Compile-time constructor checks and unit-test suites
  `AP_R3_CORE_006_ResultCreationAndEmplacement`,
  `AP_R3_CORE_006_ReplacementTransitions`, and
  `AP_R3_CORE_006_VoidReplacement`.
- Unit Tests: `AP_R3_CORE_006_ResultCreationAndEmplacement.ConvertibleErrorRequiresExplicitConstruction`; `AP_R3_CORE_006_ResultCreationAndEmplacement.DirectConstructionSelectsValueOrError`; `AP_R3_CORE_006_ResultCreationAndEmplacement.DirectConstructionSupportsMoveOnlyAlternatives`; `AP_R3_CORE_006_ResultCreationAndEmplacement.SameTypesUseDirectConstructionForValue`; `AP_R3_CORE_006_ResultCreationAndEmplacement.VoidResultSupportsDirectErrorConstruction`; `AP_R3_CORE_006_ResultCreationAndEmplacement.FactoryFunctionsSupportInPlaceConstruction`; `AP_R3_CORE_006_ResultCreationAndEmplacement.EmplaceValueAndErrorReplaceActiveAlternative`; `AP_R3_CORE_006_ResultCreationAndEmplacement.VoidResultSupportsEmplacement`; `AP_R3_CORE_006_ReplacementTransitions.PreservesPayloadOnFailureAndReplacesOnSuccess`; `AP_R3_CORE_006_VoidReplacement.PreservesStateOnFailureAndReplacesOnSuccess`
- Deviation: None.

## AP-R3-CORE-007 - Result state query

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `HasValue()` shall return `true` if and only if the result
  contains a value or, for `Result<void, E>`, represents successful completion.
- Requirement: `operator bool()` shall return the same result as `HasValue()`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, pp. 65-66,
  SWS_CORE_00751 and SWS_CORE_00752](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, pp. 9-10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Unit test
  `AP_R3_CORE_007_HasValueAndBoolReportState` ✅.
- Unit Tests: `AP_R3_CORE_007_HasValueAndBoolReportState_ValueStates.ReportsSelectedState`; `AP_R3_CORE_007_HasValueAndBoolReportState_VoidStates.ReportsSelectedState`; `AP_R3_CORE_007_HasValueAndBoolReportState.FalsePayloadReportsSuccess`; `AP_R3_CORE_007_HasValueAndBoolReportState.IdenticalPayloadsReportDifferentStates`; `AP_R3_CORE_007_HasValueAndBoolReportState.VoidZeroErrorReportsFailure`; `AP_R3_CORE_007_HasValueAndBoolReportState.QueriesFollowValueReplacement`; `AP_R3_CORE_007_HasValueAndBoolReportState.QueriesFollowVoidReplacement`; `AP_R3_CORE_006_ReplacementTransitions.PreservesPayloadOnFailureAndReplacesOnSuccess`; `AP_R3_CORE_006_VoidReplacement.PreservesStateOnFailureAndReplacesOnSuccess`
- Deviation: None.

## AP-R3-CORE-008 - Value access and exception conversion

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `Value()` shall provide access to the stored value only when
  `HasValue()` is `true`. Calling `Value()` on an error result shall be treated
  as a violation and shall terminate the process.
- Requirement: When C++ exceptions are enabled, `ValueOrThrow()` shall return
  the stored value when the result contains a value. When the result contains an `ErrorCode`, it shall invoke the
  associated `ErrorDomain` exception conversion.
- Requirement: When C++ exceptions are enabled, every concrete `ErrorDomain`
  shall define an exception type and shall convert its own `ErrorCode` objects
  to that exception type.
- Requirement: When C++ exceptions are disabled, `ValueOrThrow()` shall not
  participate in overload resolution, for both `Result<T, E>` and
  `Result<void, E>`. Throwing conversion implementations shall be excluded
  from exception-disabled compilation.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.3, p. 25,
  SWS_CORE_00003; §7.2.1.5, pp. 30-31; §7.2.1.7.2-7.2.1.7.3,
  pp. 32-33, SWS_CORE_10910 and SWS_CORE_10953; §8.1.1, p. 53,
  SWS_CORE_00154; §8.1.4, pp. 67-68 and 73-74,
  SWS_CORE_00755 to SWS_CORE_00756 and SWS_CORE_00766 to
  SWS_CORE_00769](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Build and run common value-access tests and death test
  `AP_R3_CORE_008_ValueOnErrorTerminates` ✅ in both configurations. Run unit test
  `AP_R3_CORE_008_ValueOrThrowConvertsDomainError` ✅ (test declaration exists;
  exception-enabled execution remains pending confirmed CI evidence).
  Compile-time checks shall verify absence of `ValueOrThrow()` with exceptions
  disabled and presence with exceptions enabled, for ordinary and void results.
  Inspect compiler commands to verify the selected mode.
- Unit Tests: `AP_R3_CORE_008_DomainConversion.NullConverterTerminates`; `AP_R3_CORE_008_DomainConversion.DispatchesCustomDomain`; `AP_R3_CORE_008_ValueAccess.ReferencesStoredValue`; `AP_R3_CORE_008_ValueAccess.MovesPayload`; `AP_R3_CORE_008_ValueAccess.VoidSuccess`; `AP_R3_CORE_008_ValueOnErrorTerminates.Terminates`; `AP_R3_CORE_008_ValueOnErrorTerminates_Const.Terminates`; `AP_R3_CORE_008_ValueOnErrorTerminates_Rvalue.Terminates`; `AP_R3_CORE_008_ValueOnErrorTerminates_Void.Terminates`; `AP_R3_CORE_008_ValueOrThrowConvertsDomainError.ConvertsError`; `AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Const.ConvertsError`; `AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Rvalue.ConvertsError`; `AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Void.ConvertsError`; `AP_R3_CORE_008_ThrowSuccess.PreservesReferences`; `AP_R3_CORE_008_ThrowSuccess.MovesPayload`; `AP_R3_CORE_008_ThrowSuccess.VoidSuccess`
- Deviation: None.

## AP-R3-CORE-009 - Error access

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `Error()` shall provide access to the stored error only when
  `HasValue()` is `false`. Calling `Error()` on a value result shall be treated
  as a violation and shall terminate the process.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.3, p. 25,
  SWS_CORE_00003; §8.1.4, pp. 68-69,
  SWS_CORE_00757 to SWS_CORE_00758](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1.2, p. 10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
- Verification: Death test
  `AP_R3_CORE_009_ErrorOnValueTerminates` ✅.
- Unit Tests: `AP_R3_CORE_009_ErrorAccess.ReturnsStoredError`; `AP_R3_CORE_009_ErrorAccess.VoidReturnsStoredError`; `AP_R3_CORE_009_ErrorOnValueTerminates.Terminates`; `AP_R3_CORE_009_ErrorAccess.MovesErrorPayload`; `AP_R3_CORE_009_ErrorAccess.VoidMovesErrorPayload`
- Deviation: None.

## AP-R3-CORE-010 - Result move state

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: Moving a `Result` shall preserve its active alternative in the
  destination object.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §8.1.4, p. 61,
  SWS_CORE_00726; p. 64,
  SWS_CORE_00742](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit test `AP_R3_CORE_010_MovePreservesState` ✅.
- Unit Tests: `AP_R3_CORE_010_MovePreservesState.ValueResultKeepsState`; `AP_R3_CORE_010_MovePreservesState.VoidResultKeepsState`; `AP_R3_CORE_005_ResultHasExactlyOneState_MoveConstructionPreservesState.MoveConstructionPreservesState`; `AP_R3_CORE_005_ResultVoidHasExactlyOneState_MoveConstructionPreservesState.MoveConstructionPreservesState`
- Deviation: The moved-from object is valid only for destruction or assignment.
  Its state and contained object are unspecified and shall not be used.

## AP-R3-CORE-011 - Exception-free Result use

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: Release 3 public operations that can report recoverable failures
  shall use `Result` rather than throwing C++ exceptions directly.
- AUTOSAR source:
  [Guidelines for using Adaptive Platform interfaces, R23-11, §3.1, p. 8;
  §3.1.2, pp. 8-10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf).
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.3, p. 25,
  SWS_CORE_00002](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit tests for every Release 3 public operation that has a
  recoverable failure path.
- Unit Tests: `AP_R3_CORE_011_ExceptionFreeResultUse.PublicFailureUsesResultState`; `AP_R3_CORE_011_ExceptionFreeResultUse.ConversionIsOptionalBoundary`
- Deviation: `ara::log` follows its specific AUTOSAR-inspired policy in
  AP-R3-LOG-008: internal logging failures are discarded rather than reported.

## AP-R3-CORE-012 - Error-domain extensibility

- Status: Implemented
- Status Deviation: Accepted deviation: the repository treats this requirement as implemented for the reviewed work, even though the optional exception-enabled verification remains a follow-up CI/build check.
- Requirement: `ErrorCode` shall represent an error from any `ErrorDomain`
  instance. Adding a new concrete `ErrorDomain` with a unique identifier shall
  not require changes to `ErrorCode` or `Result`.
- AUTOSAR source:
  [Specification of Adaptive Platform Core, R23-11, §7.2.1.4.1-7.2.1.4.2,
  pp. 26-28, SWS_CORE_10302 and
  SWS_CORE_10303](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf).
- Verification: Unit test
  `AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains` ✅.
- Unit Tests: `AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains.RetainsValue`; `AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_RetainsOriginatingDomain.RetainsOriginatingDomain`; `AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_DistinguishesSameValueFromDifferentDomain.DistinguishesSameValueFromDifferentDomain`
- Deviation: Release 3 supplies one concrete domain. Later domains are
  project-defined rather than ARXML-generated.