# ADR 0006: Disable exceptions by default and support both build modes

## Status

Accepted. Build integration and implementation changes are pending; the current
targets are not yet verified to compile with exceptions disabled.

## Context

AdaptivePi uses explicit error handling to teach propagation and recovery.
An optional exception-enabled configuration also teaches domain-specific
exceptions, stack unwinding, and exception safety.

AUTOSAR Adaptive supports both workflows. The R23-11
[Interface Guidelines, section 3.1, pages 8–10](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
describe Result-based handling without compiler exception support and optional
conversion through ValueOrThrow. The R23-11
[Architectural Decisions, page 24](https://www.autosar.org/fileadmin/standards/R23-11/FO/AUTOSAR_FO_EXP_SWArchitecturalDecisions.pdf)
explain that ValueOrThrow has no available overloads without exception support.
This is not a universal AUTOSAR ban on exceptions or a claim about OEM policies.

## Decision

- Repository builds default to disabled exceptions using `-fno-exceptions` on
  GCC/Clang, for both normal host development and target builds.
- Provide a CMake option `ADAPTIVE_PI_ENABLE_EXCEPTIONS`, defaulting to `OFF`.
  Selecting `ON` enables compiler exceptions and the corresponding APIs without
  editing implementation sources.
- Recoverable application failures use Result in both configurations
  (AP-R3-CORE-011). Explicit exception-conversion APIs are an opt-in boundary.
- Invalid Value/Error access terminates in both modes (AP-R3-CORE-008/009).
- ValueOrThrow is unavailable when exceptions are disabled, including for void
  results. Do not substitute a terminating fallback.
- Compile domain-specific exceptions and throwing converters only when exceptions
  are enabled. Domain identity, ErrorCode, and ordinary Result operations remain
  available in both configurations.
- Use a consistent mode across project libraries and consumers. Do not mix
  differently configured definitions of the same headers in one executable.
- Verify both modes in separate build directories and CI jobs. Common tests run
  in both; throwing-constructor and exception-conversion tests run only in the
  enabled configuration. Keep successful replacement coverage in both.

## Implementation path

These are pending production changes, not an already available build option.

1. Add the CMake option and propagate compiler options through target usage
   requirements, including consumers of public template headers. Select
   `-fexceptions` for the enabled GCC/Clang build. Diagnose unsupported compilers
   rather than silently ignoring the policy.
2. Centralize feature detection in a configuration header. Check actual compiler
   exception support and diagnose conflicts with the selected mode. Preprocess
   away throwing code and entire ValueOrThrow declarations: a runtime condition
   does not hide a throw from an exception-disabled compiler.
3. Guard exception types, converters, and converter registration consistently.
   Preserve constexpr domain identity in both configurations.
4. Finish checked value access and conditional conversion for requirement 008.
   Throwing conversion operations must not be noexcept.
5. Adapt exception-specific test support and bodies. Add compile-time API checks,
   success and death tests in both modes, and exception conversion tests in the
   enabled mode. Preserve existing throwing-emplacement tests for 005–007.
6. Verify compile commands for libraries, applications, and tests. Audit
   dependencies: project flags do not rebuild precompiled dependencies.

## Verification matrix

| Check | Exceptions disabled (default) | Exceptions enabled (optional) |
|---|---|---|
| GCC/Clang compiler mode | `-fno-exceptions` | `-fexceptions` |
| Construction, state, access, successful replacement | Run | Run |
| Invalid Value/Error access | Death tests | Death tests |
| ValueOrThrow API | Compile-time absence | Compile-time presence |
| Domain conversion, including void Result | Excluded | Run |
| Replacement when payload construction throws | Excluded | Run |

## Consequences

The default workflow teaches explicit error handling while the optional build
preserves the exception-related learning goals. Requirement 008 states the
configuration-dependent contract. Requirements remain Approved until verified;
this decision does not mark implementation complete.

With exceptions disabled, throwing constructors cannot provide recoverable
failure paths. Perform fallible work through explicit result-returning APIs.
Disabling exceptions does not make allocation or standard-library failure
recoverable. The replacement implementation can retain its exception-enabled
guarantees without depending on throws in the default configuration.

Disabling exceptions does not establish bounded execution time or functional
safety. Process termination is not itself a safe vehicle response; supervision
and fallback belong to system architecture. AdaptivePi does not claim production
ADAS qualification or AUTOSAR conformance.
