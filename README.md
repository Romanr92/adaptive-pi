# AdaptivePi

AdaptivePi is an **Adaptive AUTOSAR-inspired embedded Linux platform demonstrator**.

It is a learning and portfolio project that recreates selected Adaptive Platform concepts in modern C++ while using a realistic embedded-Linux workflow: Yocto, a PREEMPT_RT kernel, ARM64 cross-compilation, QEMU deployment, SSH, and remote debugging.

The project deliberately does **not** use proprietary AUTOSAR implementation code. It provides a focused, educational implementation of selected concepts and interfaces.

## Current status

Coverage and ESBMC reports: https://romanr92.github.io/adaptive-pi/

**Release 3 complete — `ara::core` and `ara::log`** ✅

**Working on Release 4 — Execution Management and Manifests**

The [Release 3 evidence report](docs/verification/release-3.md) records passing
native tests, both CI exception configurations, and ARM64 QEMU smoke checks.

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

### Release 3 — `ara::core` and `ara::log` ✅

- Implemented project-owned error domains, error codes, and value/error results,
  including `Result<void>` and optional exception conversion.
- Implemented context-based console logging, severity filtering, bounded streams,
  modelled messages, and compile-time trace routing.
- Migrated `hello-adaptive` to Result-based initialization and structured logging.
- Verified 445 native tests without skips, both exception configurations in PR
  CI, the ARM64 cross-build, and application and logging smoke tests in QEMU.
- Added API guides, requirement verification evidence, and numbered component
  design decisions traced by comments in the code.

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

Formatting requires **clang-format 23.1.1**, matching CI. Follow the
[pinned formatter installation steps](docs/guides/fresh-arch-yocto-build.md#install-the-pinned-c-formatter)
for native Arch or WSL Arch, and configure the editor to use that executable.

Format entire project according to rules:
```bash
find apps platform -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i
```

or

```Bash
git ls-files -z -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' '*.ipp' '*.inl' | xargs -0 -r clang-format -i
```

## Build presets

| Preset | Purpose |
|---|---|
| `debug-app` | Native Arch/Linux application build |
| `debug-unit-tests` | Native unit-test build and execution |
| `debug-esbmc-proofs` | ESBMC installation and all registered platform/application proofs |
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
| Release 4 | ESBMC bounded proofs for manifest validation, lifecycle transitions, restart-policy decisions, and data-model invariants |
| Release 5 | Two-process communication, provider-loss and reconnect integration evidence (#28) |
| Release 6 | Crash-to-DTC integration, persistence and QEMU evidence (#34) |
| Release 7 | End-to-end DoIP/UDS tests, crash-record reads and application restart routines (#40) |
| Additional quality work (tracking pending) | Broader integration and lifecycle recovery checks, and an expanded CI gate; retained roadmap goals without dedicated GitHub issue/milestone coverage |

ESBMC will not be used as a blanket check for the whole project. It will verify
selected bounded safety properties where model checking is useful and practical.
Host CI runs the registered baseline through the named **ESBMC proofs** job
using Z3. Release 4 harness expansion remains tracked by #41. See the
[ESBMC guide](docs/guides/esbmc.md#pull-request-gate-and-branch-protection) for
merge enforcement and the documented branch-administration permission limit.

## Current development — Release 4: Execution Management and Manifests

Release 4 will install, launch and supervise one self-contained application package
through a documented deployment workflow and systemd integration.
The scope follows the [Release 4 milestone](https://github.com/Romanr92/adaptive-pi/milestone/2)
issues below, extended during requirements review to include documented package
installation. Issue #17 still lists installation as excluded and needs a scope
update to match this reviewed direction.

| Issue | Planned work |
|---|---|
| [#17](https://github.com/Romanr92/adaptive-pi/issues/17) | Define a versioned manifest and package layout: application identity, executable, arguments, working directory, restart policy, optional environment and service-unit name, with validation rules and examples. |
| [#18](https://github.com/Romanr92/adaptive-pi/issues/18) | Parse and validate manifests independently of process control; report project-owned errors for unreadable, malformed, unsupported-version, or invalid-path inputs. |
| [#19](https://github.com/Romanr92/adaptive-pi/issues/19) | Add systemd-backed start, stop, and active-state queries, with project-owned errors and a fake controller for deterministic host tests. |
| [#20](https://github.com/Romanr92/adaptive-pi/issues/20) | Track initializing, running, terminating, failed, and restarting states; distinguish controlled stops from crashes and apply the manifest restart policy. |
| [#21](https://github.com/Romanr92/adaptive-pi/issues/21) | Package `platform-test-service` with a manifest and systemd unit, log startup, and provide controlled test-only fault injection and QEMU deployment. |
| [#22](https://github.com/Romanr92/adaptive-pi/issues/22) | Collect passing host/CI and ARM64 evidence, demonstrate manifest-driven startup followed by an intentional crash and automatic restart in QEMU, and document APIs and limitations. |
| [#41](https://github.com/Romanr92/adaptive-pi/issues/41) | Add bounded ESBMC proofs for feasible Release 4 logic, including invalid-manifest rejection, permitted lifecycle transitions, and restart-policy safety, with explicit assumptions and results. |
| [#72](https://github.com/Romanr92/adaptive-pi/issues/72) | Minimize the unified development/runtime Yocto image after functional verification, record before/after measurements, and repeat the QEMU scenario before #71. |
| [#71](https://github.com/Romanr92/adaptive-pi/issues/71) | Exercise the minimized Release 4 image and crash/restart/controlled-stop scenario on a Raspberry Pi, recording hardware and footprint evidence. |
| [#73](https://github.com/Romanr92/adaptive-pi/issues/73) | Extend logging with configurable bindings, bounded rotating files, journald feasibility, bounded delivery and a minimal lifecycle log; review the [Draft logging requirements](docs/requirements/release-4-logging.md) before implementation. |

The completion demonstration will show the manager starting `platform-test-service`
from its manifest, detecting an intentional crash, recording failed/restarting
transitions, and successfully restarting it. Controlled shutdown must avoid an
automatic restart.

The Raspberry Pi smoke test is an experiment rather than a hard Release 4
completion gate; hardware-specific limitations may remain open, while any
platform-independent defects become blocking issues. QEMU remains the primary
verification target, and formal Raspberry Pi support stays in the later hardware
release. Image minimization retains one image for development and runtime.

Manifests and interfaces remain project-owned and clean-room. Release 4 excludes
AUTOSAR ARXML, package signing, update management and full AUTOSAR Update and
Configuration Management, multi-application dependency graphs, distributed
supervision, and diagnostics integration. Bounded proofs
cover selected project logic; they do not verify systemd or the Linux kernel.

## Next releases

### Release 5 — Custom `ara::com`

Scope: [Release 5 milestone](https://github.com/Romanr92/adaptive-pi/milestone/3),
issues #23–#28 and [#44](https://github.com/Romanr92/adaptive-pi/issues/44).

- Implement a focused `ara::com`-inspired publish/subscribe interface.
- Add project-owned, clean-room SOME/IP-inspired communication for method request/response, events, and fields, without claiming AUTOSAR SOME/IP conformance.
- Define proxy/skeleton ownership, serialization, wire format and compatibility limits.
- Add local service registration, discovery and availability notifications.
- Handle timeouts, service loss, and reconnect after provider restart.
- Validate communication between separate Linux processes with host, integration, ARM64 and QEMU evidence.

### Release 6 — Diagnostic Manager Internals

Scope: [Release 6 milestone](https://github.com/Romanr92/adaptive-pi/milestone/4),
issues #29–#34.

- Run `diagnostic-manager` as an Execution-Managed application with a project-owned `ara::diag`-inspired event boundary.
- Persist diagnostic trouble codes (DTCs) and freeze-frame records, with retrieval, clear, corruption and storage-failure handling.
- Include application lifecycle state and crash/restart history from Execution Management.
- Collect image/application versions, CPU, memory, temperature, network and service health, with explicit unavailable states.
- Demonstrate an intentional application crash producing a persistent DTC and freeze frame through host integration and QEMU verification.
- Keep external DoIP/UDS protocols in Release 7.

### Release 7 — DoIP and UDS over Ethernet

Scope: [Release 7 milestone](https://github.com/Romanr92/adaptive-pi/milestone/5),
issues #35–#40.

- Implement a simplified DoIP/UDS diagnostic server backed by Release 6 Diagnostic Manager data.
- Use the Raspberry Pi Ethernet interface as a development bench connection, with a laptop command-line client and host/QEMU connectivity checks.
- Add DoIP discovery, vehicle identification, routing activation and alive checks.
- Add UDS dispatch, sessions, timing, negative responses, DTC/data reads, and restricted security, write and routine-control demonstrations.
- Verify that a laptop can read the DTC/freeze frame from an intentional crash and run an application restart routine.
- Document the differences between standard Raspberry Pi Ethernet and production Automotive Ethernet technologies such as 100BASE-T1. This retained goal needs explicit GitHub issue coverage.

### Release 8 — Raspberry Pi deployment (GitHub tracking pending)

This roadmap release is retained. GitHub currently has no Release 8 milestone or
dedicated formal hardware-support issues. Release 4 [#71](https://github.com/Romanr92/adaptive-pi/issues/71)
provides an early smoke experiment and explicitly leaves formal support to a later
hardware release.

- Add Raspberry Pi Yocto support.
- Build and deploy the AdaptivePi image to real hardware.
- Validate Ethernet communication, services, deployment workflow, and measured timing behavior.

### Additional integration and quality goals (GitHub tracking pending)

These goals from the earlier Release 7 roadmap are retained. Releases 5–7 already
include scoped integration, fault injection and restart demonstrations; broader
coverage and CI expansion still need explicit issue ownership and release placement.

- Add integration tests across applications and processes.
- Add fault handling and lifecycle recovery behavior.
- Extend CI with formatting, static analysis, unit tests, and integration checks where practical.

## Documentation

- [WSL2 Arch host on a non-C drive](docs/guides/wsl-arch-yocto-host.md)
- [Fresh Arch Linux to Yocto/QEMU setup](docs/guides/fresh-arch-yocto-build.md)
- [Yocto and PREEMPT_RT image build](docs/guides/yocto-qemu-preempt-rt-build-guide.md)
- [SDK cross-build, deployment, and QEMU debugging](docs/guides/sdk-cross-build-and-qemu-deployment.md)
- [Architecture Decision Records](docs/adr/)
- [Logger foundation and factory contract](docs/guides/ara-log-foundation.md)
- [Core API and error-handling guide](docs/guides/ara-core.md)
- [Release 3 verification evidence](docs/verification/release-3.md)
- [Core component design decisions](platform/ara-core/doc/design-decisions.md)
- [Logging component design decisions](platform/ara-log/doc/design-decisions.md)
- [Exception build policy](docs/adr/0006-exception-build-policy.md)
- [Per-application unit tests](docs/guides/per-application-unit-tests.md)
- [Step-by-step ESBMC proof training](docs/guides/esbmc-proof-training.md)
- [Local ESBMC HTML coverage reports](tools/esbmc-report/README.md)
- [ESBMC concepts, proof authoring, and execution](docs/guides/esbmc.md)
- [Publishing and installing a complete Yocto platform](docs/guides/publish-platform-release.md)
- [Publishing a versioned Yocto SDK](docs/guides/publish-yocto-sdk.md)

## License

Copyright 2026 Razvan Stefan Roman.

Licensed under the [Apache License, Version 2.0](LICENSE).
