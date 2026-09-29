# ESBMC installation and bounded proofs

Native CMake configuration finds a host `esbmc` executable and runs every
explicitly registered proof under both `platform/` and `apps/`. If ESBMC is
missing, `scripts/install-esbmc.sh` downloads the official **8.5** release,
checks its pinned SHA-256, and installs the complete distribution under
`<build-directory>/tools/esbmc`. No root privileges or PATH changes are needed.

The installer supports Linux x86_64 and ARM64 hosts and requires Bash, curl,
unzip, coreutils, and util-linux (`flock`). The Linux x86_64 release has been
smoke-tested on the Arch development host. ARM64 installation has not been
executed locally. Downloads come from the
[official release](https://github.com/esbmc/esbmc/releases/tag/v8.5).

```bash
cmake --preset debug-esbmc-proofs
cmake --build --preset debug-esbmc-proofs
```

In VS Code, click the configure-preset entry in the status bar and choose
**Debug: ESBMC proofs**. The workspace already enables CMake presets and keeps
that selector visible. Alternatively run **CMake: Select Configure Preset** from
the Command Palette. Select the matching build preset if prompted. Configuring
runs the proofs; the Build button uses the matching `run-esbmc` target to rerun
only the proofs. The proof preset has its own build directory, disables unit-test
setup and clang-tidy, and requires a non-empty proof suite.

An existing executable is reused and checked with `--version`. Set
`-DESBMC_EXECUTABLE=/absolute/path/to/esbmc` to select an installation explicitly.
The installer pins downloads; an existing user-supplied version is not replaced.
For reproducible CI, install the pinned version in a fresh directory and select
that binary explicitly. Installation can also be run independently:

```bash
scripts/install-esbmc.sh build/tools/esbmc
cmake --preset debug-app \
  -DESBMC_EXECUTABLE="$PWD/build/tools/esbmc/bin/esbmc"
```

Installation uses a lock and a temporary directory, publishes only a validated
installation, and refuses to overwrite an incomplete destination. A working
installation is reused without network access. Download, checksum, and startup
errors stop configuration.

## Register component proofs

Use a `proofs/CMakeLists.txt` inside any component under either root:

```text
platform/ara-core/proofs/CMakeLists.txt
platform/ara-core/proofs/proof_result_state.cpp
apps/hello-adaptive/proofs/CMakeLists.txt
apps/hello-adaptive/proofs/proof_application_invariant.cpp
```

The central runner discovers these manifests. Component CMake files should not
also add their `proofs/` subdirectory. Only harnesses explicitly registered in
the manifests run; an unregistered `.cpp` file is not automatically verified.
Not every component must have proofs.

Example manifest, to use once the named harness exists:

```cmake
add_esbmc_proof(ara-core-result-state
    SOURCES proof_result_state.cpp
    INCLUDE_DIRECTORIES ../include
    UNWIND 8
    TIMEOUT 60
)
```

Names must be globally unique and contain only letters, digits, underscores, or
hyphens. `SOURCES` may include implementation files required by the harness;
compiled CMake libraries are not automatically linked into ESBMC. Include paths
and source paths are relative to the proof manifest. `DEFINITIONS` accepts
macros without `-D`; `OPTIONS` accepts additional ESBMC arguments. Document each
property, input assumptions, bound, timeout, additional options, and excluded
behaviour beside its harness. Do not use options that suppress the property
being checked or unwinding failures to manufacture a passing result.

The runner uses C++17, enables memory-leak checks, retains ESBMC's default
pointer/bounds checks and unwinding assertions, and passes the selected
`ADAPTIVE_PI_ENABLE_EXCEPTIONS` mode as both a frontend flag and macro. ESBMC
8.5 has `--no-pointer-check`, not `--pointer-check`; pointer checking is on by
default. Proofs are host analysis inputs and are not compiled into applications.
Local verification uses exceptions disabled; exception-enabled evidence belongs
in CI, consistent with the project's exception policy.

## Results and quality-gate mode

Every registered proof is attempted, even after an earlier proof fails. CMake
prints its name and command, writes the command, exit result, stdout, and stderr
to `<build-directory>/esbmc/logs/<proof-name>.log`, and fails the command if any
proof times out, cannot execute, returns a nonzero status, or lacks ESBMC's
`VERIFICATION SUCCESSFUL` result. Configuration and the `run-esbmc` build target
use the same generated command manifest and runner. The target always reruns
proofs, including after editing a harness without changing its manifest.

For a strict local or future CI quality gate:

```bash
cmake --preset debug-esbmc-proofs
cmake --build --preset debug-esbmc-proofs
```

The preset sets `ADAPTIVE_PI_ENABLE_ESBMC=ON` and
`ADAPTIVE_PI_ESBMC_REQUIRE_PROOFS=ON`. Strict mode rejects disabled verification
and an empty proof suite. Other native configurations run registered proofs too;
if none are registered, they warn that no project properties were verified.

The initial suite contains two supplementary proofs, both verified locally with
ESBMC 8.5 on Linux x86_64 and exceptions disabled:

| Proof | Property | Unwind / timeout | Result |
|---|---|---|---|
| `ara-core-error-code` | AP-R3-CORE-003 value and originating-reference retention; AP-R3-CORE-004 equality by value and domain ID | 32 / 60 seconds | Passed |
| `platform-test-service-build-info` | Metadata view lengths and contents match the application's existing contract for every valid symbolic index | 32 / 60 seconds | Passed |

See each component's `proofs/README.md` for assumptions and exclusions.
These do not establish the Release 4 harness baseline tracked by
[issue #41](https://github.com/Romanr92/adaptive-pi/issues/41).
This integration implements the installation and local CMake runner portion of
[issue #42](https://github.com/Romanr92/adaptive-pi/issues/42), including the
component-local layout discussed in its comment. It does not establish the PR
workflow or branch-protection gate; those require the supported proof baseline.

Use `-DADAPTIVE_PI_ENABLE_ESBMC=OFF` to disable installation and proofs for a
build. ESBMC defaults to off for target/cross builds; if explicitly enabled,
the executable is still searched on the host rather than in the target sysroot.
Existing CMake cache choices take precedence over defaults.

Passing bounded proofs establishes only the documented properties under their
assumptions and bounds. It does not verify the complete application, operating
system, hardware, or excluded third-party code, and does not replace unit tests
or target verification.
