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
```

A top-level `tests/` directory is reserved for future cross-application integration tests and system tests. It must not contain unit tests that belong to one application.

## Build policy

AdaptivePi retains exactly two CMake configure presets:

- `debug-app` sets `BUILD_TESTING=OFF`. Application sources build, but test directories are not entered and GoogleTest is not fetched.
- `debug-unit-tests` sets `BUILD_TESTING=ON`. Each application's `tests/` directory is entered, its unit-test executable is built, and CTest runs the discovered tests.

Each application conditionally includes its tests:

```cmake
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

The repository root owns the common test runner because it must run all discovered application tests through CTest.

## Why this structure

- Application code and its tests evolve together.
- A new service can be reviewed, built, and tested as one self-contained unit.
- Normal application builds remain fast and do not download or build GoogleTest.
- Future cross-service tests remain clearly distinct from application unit tests.

## Verification

The structure was verified on the Arch Linux host.

```bash
cmake --preset debug-app
cmake --build build/debug-app -j 1

cmake --preset debug-unit-tests
cmake --build build/debug-unit-tests --target run-unit-tests -j 1
```

Results:

- The `debug-app` preset configured and built without adding test directories.
- The `debug-unit-tests` preset configured the application-owned test directory.
- CTest discovered and passed both `BuildInfoTest` cases.