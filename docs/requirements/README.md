# AdaptivePi Release 3 requirements baseline

## Purpose

This directory defines the requirements baseline for Release 3: a small,
clean-room educational subset inspired by AUTOSAR Adaptive Platform
`ara::core` and `ara::log`.

AdaptivePi does not claim AUTOSAR conformance. The requirements are written in
project-owned wording and do not reproduce AUTOSAR specification text or
implementation code.

## Requirement identifiers

- `AP-R3-CORE-xxx`: `ara::core`-inspired requirements
- `AP-R3-LOG-xxx`: `ara::log`-inspired requirements

Requirement identifiers are stable and shall not be reused for a different
requirement.

## Status

- `Draft`: proposed and subject to review
- `Approved`: accepted as the Release 3 implementation baseline
- `Implemented`: implemented and verified

No Release 3 C++ implementation shall begin until every requirement in this
baseline is `Approved`.

## Traceability convention

Each requirement shall contain:

1. a stable AdaptivePi requirement ID;
2. status;
3. an atomic normative requirement;
4. AUTOSAR source trace, when applicable;
5. verification method and planned test;
6. deliberate scope deviation.

An AUTOSAR source trace shall contain:

- official AUTOSAR R23-11 document link;
- document section;
- PDF page;
- AUTOSAR requirement ID, where one exists.

Requirements that are solely AdaptivePi design decisions shall explicitly state
that they have no direct AUTOSAR requirement source.

## Reviewed AUTOSAR sources

- [Specification of Adaptive Platform Core, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_Core.pdf)
- [Specification of Log and Trace, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_LogAndTrace.pdf)
- [Guidelines for using Adaptive Platform interfaces, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_EXP_InterfacesGuidelines.pdf)
- [Specification of Platform Types for Adaptive Platform, R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_PlatformTypes.pdf)

The Platform Types specification was reviewed for scope alignment. No Release 3
requirement directly traces to it because Release 3 does not implement AUTOSAR
model types, generated artifacts, or platform type definitions.

## Release 3 exclusions

Release 3 excludes:

- full AUTOSAR API compatibility;
- generated AUTOSAR model and configuration artifacts;
- DLT and remote log transport;
- DLT transport, persistence, runtime trace configuration, external trace-tool
  integration, and filtering configuration;
- `ara::com`, Execution Management, manifests, diagnostics, and hardware
  support;
- `Future`, `Promise`, `Optional`, and other `ara::core` utilities not listed
  in the Release 3 requirements.

## Release 3 modelled messages and tracing

Release 3 implements manually authored C++ definitions for modelled log messages
and compile-time trace routing. It does not generate message definitions,
trace-routing configuration, manifests, or trace-tool specializations from
ARXML.

The Release 3 trace artifact interface is an educational clean-room boundary.
It supports unit-test verification of logger, trace-artifact, combined, and
discard routing. It does not integrate with a production trace tool.

## Source coverage

| AUTOSAR source | Release 3 use | Requirement location |
|---|---|---|
| Adaptive Platform Core | Defines ErrorDomain, ErrorCode, Result semantics and access behaviour. | `release-3-ara-core.md` |
| Log and Trace | Defines logger contexts, levels, logger ownership, and default console behaviour. | `release-3-ara-log.md` |
| Interfaces Guidelines | Guides exception-free ErrorCode and Result usage. | `release-3-ara-core.md` |
| Platform Types | Reviewed; no applicable Release 3 implementation scope. | This README |