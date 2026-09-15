# AdaptivePi

AdaptivePi is a clean-room, Adaptive AUTOSAR-inspired automotive platform demonstrator for Raspberry Pi.

It is a portfolio and learning project that explores modern C++/Linux, service-oriented automotive software, diagnostics, deployment, and target debugging. It is **not** an AUTOSAR implementation and does not claim Adaptive AUTOSAR compliance.

## Current status

Release 0 is in progress: repository, host-native build, unit tests, formatting, static analysis, and CI.

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

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Run the service:

```bash
./build/debug/apps/platform-test-service/platform-test-service
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

- **Release 0:** Repository, CMake/Ninja, GoogleTest, formatting, static analysis, and CI
- **Release 1:** Yocto image for QEMU AArch64, then Raspberry Pi 3
- **Release 2:** Cross-build, deployment over SSH, and remote GDB
- **Release 3:** Manifest-driven execution management
- **Release 4:** Focused custom `ara::core`, `ara::log`, `ara::exec`, and `ara::diag`
- **Later releases:** Service communication, diagnostics, camera pipeline, sign detection, and HMI

## Quality gates

Every change should pass:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
find apps tests -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror
```

## License

Copyright 2026 Razvan Stefan Roman.

Licensed under the [Apache License, Version 2.0](LICENSE).