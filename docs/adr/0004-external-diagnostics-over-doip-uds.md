# ADR 0004: Use DoIP and UDS for external diagnostics

- Status: Accepted
- Date: 2026-09-15

## Context

AdaptivePi needs an externally visible diagnostic interface that resembles automotive ECU diagnostics while remaining feasible on embedded Linux.

## Decision

Use Ethernet-based Diagnostics over IP (DoIP) as the transport and Unified Diagnostic Services (UDS) as the diagnostic protocol for external diagnostic access.

The laptop-based `diagnostic-client` will communicate with `diagnostic-manager` running on the target.

## Consequences

### Positive

- Uses recognizable automotive diagnostic concepts
- Ethernet supports remote testing and demonstrable client/server behavior
- Separates external diagnostics from internal service health reporting

### Negative

- DoIP and UDS protocol implementation must be scoped carefully
- Ethernet behavior and interoperability require physical Raspberry Pi validation
- The implementation will be a learning demonstrator, not a production-certified diagnostic stack