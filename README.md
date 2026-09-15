# AdaptivePi

AdaptivePi is a clean-room, Adaptive AUTOSAR-inspired automotive platform demonstrator for Raspberry Pi.

It is a portfolio and learning project that explores modern C++/Linux, service-oriented automotive software, diagnostics, deployment, and target debugging. It is **not** an AUTOSAR implementation and does not claim Adaptive AUTOSAR compliance.

## Current status

- **Release 0 — Project foundation:** Complete. Added the CMake/Ninja C++ project structure, Clang-based host builds, GoogleTest, clang-format, clang-tidy, GitHub Actions host CI, architecture documentation, and initial ADRs. ✅

- **Phase 0.5 — VS Code workflow:** Complete. Added two focused CMake Debug presets: one for building the application and one for building and running unit tests. Configured the VS Code CMake Tools status bar and launch workflow for the Remote–SSH Arch Linux environment. ✅

- **Release 1 — Yocto and QEMU AArch64 environment:** Planned. Create a reproducible Yocto-based Linux image, boot it in QEMU, and establish a target-side development and validation workflow. 

- **Later release automation:** Planned. GitHub Actions will create versioned Raspberry Pi deployment artifacts after the Yocto target image and deployment workflow exist.

The current executable is:

- `platform-test-service`

## Host build and test

Prerequisites:

- CMake
- Ninja
- Clang
- clang-format
- clang-tidy

Configure, build, and test:

### Debug application

```bash
cmake --preset debug-app
cmake --build build/debug-app --parallel
```

### Debug unit tests

```bash
cmake --preset debug-unit-tests
cmake --build build/debug-unit-tests --parallel
```

The Unit Tests build runs CTest/GoogleTest automatically. To rerun tests without building, use:

```bash
ctest --preset debug-unit-tests
```

Run the service:

```bash
./build/debug-app/apps/platform-test-service/platform-test-service
```

GoogleTest is automatically downloaded as a pinned dependency into `tools/gtest` during the first test-enabled CMake configuration. It is intentionally not committed to this repository.

## Architecture direction

AdaptivePi will evolve into a small process-based platform with custom, focused `ara::`-inspired APIs and manifest-driven application lifecycle management.

Target runtime services:

- `platform-test-service`
- `diagnostic-manager`
- `camera-service`
- `traffic-sign-detection-service`
- `sign-hmi-service`

The early development path is:

```text
Host-native tests → QEMU AArch64 integration → Raspberry Pi hardware validation
```

## Roadmap

- **Release 0:** Repository, CMake/Ninja, GoogleTest, formatting, static analysis, and host CI
- **Phase 0.5:** VS Code CMake Tools workflow for Debug application builds and unit-test execution
- **Release 1:** Yocto image for QEMU AArch64, then Raspberry Pi 3
- **Release 2:** Cross-build, deployment over SSH, and remote GDB
- **Release 3:** Manifest-driven execution management
- **Release 4:** Focused custom `ara::core`, `ara::log`, `ara::exec`, and `ara::diag`
- **Later releases:** Service communication, diagnostics, camera pipeline, sign detection, and HMI
- **Later release automation:** GitHub Actions will create versioned Raspberry Pi deployment artifacts after the Yocto target image and deployment workflow exist

## Quality gates

Every change should pass:

```bash
cmake --preset debug-app
cmake --build build/debug-app --parallel

cmake --preset debug-unit-tests
cmake --build build/debug-unit-tests --parallel

find apps tests -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror
```

## License

Copyright 2026 Razvan Stefan Roman.

Licensed under the [Apache License, Version 2.0](LICENSE).