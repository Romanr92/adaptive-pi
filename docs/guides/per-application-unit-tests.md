# Per-Application Unit Test Structure

## Status

Implemented and verified on 2026-09-16.

## Decision

Each application owns its unit tests.

```text
apps/
  platform-test-service/
    include/
    src/
    tests/
      CMakeLists.txt
      build_info_test.cpp
    CMakeLists.txt

  hello-adaptive/
    src/
    systemd/
    tests/
      CMakeLists.txt
    CMakeLists.txt

## Verification

The structure was verified on the Arch Linux host.

```bash
cmake --preset debug-app
cmake --build build/debug-app -j 1

cmake --preset debug-unit-tests
cmake --build build/debug-unit-tests --target run-unit-tests -j 1