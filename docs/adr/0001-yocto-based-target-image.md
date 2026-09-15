# ADR 0001: Use Yocto for target Linux images

- Status: Accepted
- Date: 2026-09-15

## Context

AdaptivePi needs a reproducible embedded Linux image for QEMU AArch64 and later Raspberry Pi 3 hardware. The project must control included packages, system services, target users, SSH access, debug tooling, and image metadata.

## Decision

Use a pinned Yocto Project configuration to build target images.

Release 1 will begin with the generic `qemuarm64` machine. A separate Raspberry Pi 3 64-bit machine configuration will follow for hardware-specific validation.

## Consequences

### Positive

- Reproducible target images and package selection
- Clear separation between host development and target runtime
- A realistic embedded-Linux workflow for cross-compilation and deployment
- Support for SDK generation and target debugging

### Negative

- Steep initial learning curve and long first build times
- QEMU does not emulate Raspberry Pi-specific peripherals accurately
- The project must maintain layers and pinned revisions over time