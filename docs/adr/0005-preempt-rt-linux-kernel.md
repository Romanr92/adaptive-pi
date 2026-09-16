# ADR 0005: Use a PREEMPT_RT Linux Kernel

## Status

Accepted

## Context

AdaptivePi hosts multiple Linux processes that will later include vehicle-facing services, diagnostics, and data-processing components. The platform needs more predictable scheduling latency than a standard general-purpose Linux kernel provides.

QEMU is used for early ARM64 boot and integration validation. It does not represent Raspberry Pi hardware timing accurately.

## Decision

AdaptivePi uses Yocto's `linux-yocto-rt` provider, based on the PREEMPT_RT Linux kernel configuration.

The target image uses systemd. Time-critical application threads will use explicit scheduling and priority policies only where justified and measured.

## Consequences

- The target gains improved scheduling determinism compared with a standard Linux kernel.
- QEMU verifies build, boot, and software integration only; it does not validate real-time latency.
- Raspberry Pi hardware validation and latency measurement are required before making real-time claims.
- PREEMPT_RT does not turn Linux into a guaranteed hard real-time system.