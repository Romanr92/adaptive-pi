# AdaptivePi

AdaptivePi is an **Adaptive AUTOSAR-inspired embedded Linux platform demonstrator**.

It is a learning and portfolio project that recreates selected Adaptive Platform concepts in modern C++ while using a realistic embedded-Linux workflow: Yocto, a PREEMPT_RT kernel, ARM64 cross-compilation, QEMU deployment, SSH, and remote debugging.

The project deliberately does **not** use proprietary AUTOSAR implementation code. It provides a focused, educational implementation of selected concepts and interfaces.

## Current status

**Release 2 complete — SDK cross-build, QEMU deployment, and remote debugging** ✅

The project has a verified development platform for ARM64 target software:

```text
Windows 11 + VS Code
        ↓ Remote SSH
Arch Linux development host
        ↓ Yocto SDK cross-build
AArch64 AdaptivePi application
        ↓ SSH / SCP
QEMU ARM64 PREEMPT_RT target
        ↓ gdbserver + SSH tunnel
VS Code source-level debugging
```

## Completed work

### Release 1 — QEMU PREEMPT_RT development platform ✅

- Built Poky Scarthgap `5.0.20` for the generic `qemuarm64` machine.
- Created the project-owned `meta-adaptive-pi` Yocto layer.
- Built `adaptive-pi-image` based on `core-image-minimal`.
- Selected the `linux-yocto-rt` kernel provider.
- Verified that the target runs the PREEMPT_RT kernel:
  - `uname -a` reports `6.6.151-rt31-yocto-preempt-rt`;
  - `/sys/kernel/realtime` reports `1`.
- Selected `systemd` as the target init system.
- Added OpenSSH, `gdbserver`, and the C++ runtime to the target image.
- Created managed QEMU lifecycle scripts with duplicate-start protection and safe shutdown behavior.

### Release 2 — SDK cross-build and remote debugging ✅

- Generated and installed a Yocto SDK for the AdaptivePi image.
- Added the `debug-qemu-app` CMake preset for AArch64 cross-compilation.
- Verified that `hello-adaptive` builds as an ARM64 executable.
- Added deployment through SSH/SCP into the running QEMU guest.
- Added VS Code remote debugging with:
  - Yocto cross-GDB;
  - guest-side `gdbserver`;
  - an SSH debug tunnel;
  - automatic build, deployment, connection, and cleanup through `F5`.
- Added per-application unit-test structure and CMake presets for native application and test builds.
- Documented the Yocto build, SDK, deployment, and debugging workflows.

## Quick development workflow

Start the QEMU development image:

```bash
scripts/qemu/start-development-image.sh
```

Build the ARM64 application:

```bash
cmake --preset debug-qemu-app
cmake --build --preset debug-qemu-app
```

Deploy and run it:

```bash
scripts/qemu/deploy-binary.sh \
  build/debug-qemu-app/apps/hello-adaptive/hello-adaptive

ssh -p 2222 root@localhost /usr/local/bin/hello-adaptive
```

For source-level debugging:

1. Start the QEMU development image.
2. Open `apps/hello-adaptive/src/main.cpp`.
3. Set a breakpoint.
4. Select `Debug: QEMU Hello Adaptive` in VS Code.
5. Press `F5`.

Stop QEMU cleanly after development:

```bash
scripts/qemu/stop-development-image.sh
```

## Build presets

| Preset | Purpose |
|---|---|
| `debug-app` | Native Arch/Linux application build |
| `debug-unit-tests` | Native unit-test build and execution |
| `debug-qemu-app` | Yocto SDK cross-build for the ARM64 QEMU guest |

## Project structure

```text
apps/                 Executable applications and their unit tests
cmake/                Shared CMake modules
docs/adr/             Architecture Decision Records
docs/guides/          Reproducible development and Yocto guides
scripts/qemu/         QEMU lifecycle, deployment, and debug scripts
yocto/meta-adaptive-pi/
                      Project-owned Yocto layer and image definition
```

## Important scope and limitations

- `qemuarm64` is a generic ARM64 virtual machine, not Raspberry Pi hardware emulation.
- QEMU validates boot, software integration, deployment, and remote debugging.
- QEMU cannot validate Raspberry Pi peripherals or real-time latency behavior.
- PREEMPT_RT improves scheduling determinism but does not make Linux a guaranteed hard real-time system.
- Real hardware deployment and latency measurements will be performed later on a Raspberry Pi.

## Quality gates roadmap

Quality is part of every release. Each gate adds evidence appropriate to the
maturity of the platform rather than treating quality as a final activity.

| Stage | Required evidence |
|---|---|
| Current host workflow | Formatting, clang-tidy, native unit tests, and Host CI |
| Pre-Release 3 (complete) | A pull-request CI job cross-builds every target-compatible application with the matching Yocto SDK and verifies each output is an AArch64 ELF binary. QEMU runtime execution is deferred. |
| Release 4 | ESBMC bounded model-checking harnesses for selected pure logic such as Result/ErrorCode invariants and Execution Management lifecycle transitions |
| Release 7 | Cross-process integration tests, fault injection, restart/recovery checks, and an expanded CI gate |

ESBMC will not be used as a blanket check for the whole project. It will verify
selected bounded safety properties where model checking is useful and practical.
Before it becomes a required check, its harnesses and bounds must be stable.

## Next releases

### Release 3 — `ara::core` and `ara::log`

- Implement a focused `ara::core` foundation.
- Implement error handling and result/value types.
- Implement structured application logging.
- Add unit tests and documented API requirements.

### Release 4 — Execution Management

- Implement a simplified `ara::exec` lifecycle model.
- Define application states and deterministic startup/shutdown behavior.
- Introduce process manifests or equivalent project-owned configuration.

### Release 5 — Communication Management

- Implement a focused `ara::com`-inspired publish/subscribe interface.
- Add service discovery concepts suitable for the demonstrator.
- Validate communication between separate Linux processes.

### Release 6 — Diagnostics over Ethernet

- Implement a simplified DoIP/UDS diagnostic manager.
- Use the Raspberry Pi Ethernet interface as a development bench connection.
- Document the differences between standard Raspberry Pi Ethernet and production Automotive Ethernet technologies such as 100BASE-T1.

### Release 7 — Integration and quality

- Add integration tests across applications and processes.
- Add fault handling and lifecycle recovery behavior.
- Extend CI with formatting, static analysis, unit tests, and integration checks where practical.

### Release 8 — Raspberry Pi deployment

- Add Raspberry Pi Yocto support.
- Build and deploy the AdaptivePi image to real hardware.
- Validate Ethernet communication, services, deployment workflow, and measured timing behavior.

## Documentation

- [Yocto and PREEMPT_RT image build](docs/guides/yocto-qemu-preempt-rt-build-guide.md)
- [SDK cross-build, deployment, and QEMU debugging](docs/guides/sdk-cross-build-and-qemu-deployment.md)
- [Architecture Decision Records](docs/adr/)
- [Per-application unit tests](docs/guides/per-application-unit-tests.md)
- [Publishing a versioned Yocto SDK](docs/guides/publish-yocto-sdk.md)

## License

Copyright 2026 Razvan Stefan Roman.

Licensed under the [Apache License, Version 2.0](LICENSE).
