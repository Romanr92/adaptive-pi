# ADR 0002: Implement a focused custom middleware subset

- Status: Accepted
- Date: 2026-09-15

## Context

AdaptivePi needs platform-level concepts for application lifecycle, errors, logging, service communication, and diagnostics. Full Adaptive AUTOSAR products are proprietary and substantially larger than the scope of this portfolio project.

## Decision

Implement a small, explicitly non-compliant, `ara::`-inspired middleware subset tailored to AdaptivePi.

The initial scope will cover:

- `ara::core`: results, errors, and asynchronous primitives
- `ara::log`: structured application logging
- `ara::exec`: lifecycle and execution-state reporting
- `ara::diag`: diagnostic event reporting and health status
- `ara::com`: only the service communication features required by later applications

## Consequences

### Positive

- Concepts are learned through implementation rather than imitation
- APIs remain small, testable, and explainable in interviews
- No proprietary AUTOSAR implementation is copied or implied

### Negative

- The implementation is not compatible with commercial Adaptive AUTOSAR stacks
- API naming must always be documented as inspired by, not compliant with, AUTOSAR
- Scope discipline is required to prevent the middleware from becoming a framework rewrite