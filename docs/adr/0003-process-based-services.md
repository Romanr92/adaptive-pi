# ADR 0003: Use process-based services

- Status: Accepted
- Date: 2026-09-15

## Context

AdaptivePi demonstrates a Linux-based, service-oriented automotive platform. The project needs independently supervised applications with observable failures, controlled restarts, and explicit lifecycle states.

## Decision

Implement each runtime capability as an independent Linux process managed through systemd and an AdaptivePi Execution Manager.

The initial target processes are:

- `platform-test-service`
- `diagnostic-manager`
- `camera-service`
- `traffic-sign-detection-service`
- `sign-hmi-service`

## Consequences

### Positive

- Fault containment is stronger than in a single-process design
- Each service can be deployed, restarted, logged, and debugged independently
- The design naturally demonstrates lifecycle and supervision concepts

### Negative

- Inter-process communication and startup sequencing add complexity
- End-to-end debugging spans process boundaries
- Resource use is higher than in a monolithic executable