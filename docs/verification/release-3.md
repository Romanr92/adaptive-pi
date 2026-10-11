# Release 3 verification evidence

Issue: [#5](https://github.com/Romanr92/adaptive-pi/issues/5).
Evidence collected on 2026-10-11 for production/test revision
`5b4cd822392905ea1d36b9d6a27a578589fcbfe2`. Documentation and design-decision comments added during this audit require
review and CI on the resulting PR revision. Release 3 implementation and runtime
verification are complete; publication remains pending. No release/tag has been
created.

## Observed results

| Gate | Evidence | Result |
|---|---|---|
| Formatting | clang-format 23.1.1, all C++ sources under apps/ and platform/ | Passed |
| Native application | debug-app preset, clang-tidy and warnings as errors enabled | Passed |
| Native tests | debug-unit-tests, exceptions OFF, clang-tidy and warnings as errors enabled | 445/445 passed, no skips in final run |
| Compile-time contracts | static assertions in built test translation units | Passed in OFF build; ON passed in PR #69 CI |
| Rejected public calls | AP_R3_LOG_010_InvalidModelledMessageParametersDoNotCompile, registered with CTest | Passed |
| ARM64 build | debug-qemu-app with ADAPTIVE_PI_BUILD_LOG_SMOKE=ON | Passed |
| ELF architecture | readelf -h on hello-adaptive and ara_log_target_smoke | Both AArch64 |
| Deployment and QEMU | project start/deploy scripts, both executables run over SSH | Both exited 0 |
| PR Host CI | [PR #69 run 38098177476](https://github.com/Romanr92/adaptive-pi/actions/runs/38098177476), identical audited file tree | All five jobs passed |
| Exceptions ON | PR #69 Build and unit test (exceptions enabled) job | Passed in CI |
| Milestone documentation | README updated | Complete |
| Release publication | Final documentation/comment revision requires commit and PR checks | Pending |

The first native run skipped the real C numeric locale test because de_DE.UTF-8
was unavailable. A temporary locale was generated with localedef; the complete
suite was then rerun with LOCPATH and passed with no skips. The final local log
is `build/debug-unit-tests/release-3-ctest.log` (generated, not committed).
Test counts include parameter cases and application tests; they are execution
inventory, not a claim of exhaustive behavioral coverage or code coverage.

No exception-enabled local execution was performed. The repository's
[unit-test skill](../../.agents/skills/write-unit-tests/SKILL.md) delegates it to
CI. PR #69 head `ad1ee12b1f9dfef4dacf3d24371e1bd2d0301e3e` has the same file tree
as audited merge commit `5b4cd822392905ea1d36b9d6a27a578589fcbfe2`
(`git diff --stat <head> HEAD` produced no differences). Its
[Host CI run](https://github.com/Romanr92/adaptive-pi/actions/runs/38098177476)
passed Formatting, Build and unit test, Build and unit test (exceptions enabled),
Cross-build QEMU target, and ESBMC proofs. Both proof-mode steps passed; the gate
integration-test step reused its cached result. This is confirmed CI evidence
for the audited code tree. The documentation and comment changes still need their PR checks.
Code annotations use component-local DD IDs; only comments changed in C++ code
after the audited revision, with behavior preserved.

## Requirement traceability

Approved text, verification methods, and deviations remain the baseline:
[core](../requirements/release-3-ara-core.md),
[logging](../requirements/release-3-ara-log.md).
The statuses below are copied from those files and have not been promoted from
test existence or a single-mode execution. CORE-006 and all logging requirements
remain Approved; other core entries retain their documented status deviations.

The implementation links identify the relevant current code. Test-source links
identify requirement-specific checks; exact declaration inventories remain in
the baseline's Unit Tests entries. Those entries were checked against test
macro declarations with no missing names. The final column counts discovered
OFF-build CTest cases containing the requirement ID; some checks exercise other
requirements too. Enabled-only paths were executed by the successful exception-enabled CI job.
The counts below refer only to the locally discovered OFF configuration.
Compiler-command inspection confirmed `-fno-exceptions` and macro value `0`
for all project translation units in the native app, test, and target builds.

| Requirement | Baseline status | Implementation | Verification sources | OFF cases passed |
|---|---|---|---|---:|
| AP-R3-CORE-001 | Implemented | [error_domain.h](../../platform/ara-core/include/ara/core/error_domain.h) | [error_domain_test.cpp](../../platform/ara-core/tests/error_domain_test.cpp) | 6 |
| AP-R3-CORE-002 | Implemented | [error_domain.h](../../platform/ara-core/include/ara/core/error_domain.h), [adaptive_pi_error_domain.h](../../platform/ara-core/include/ara/core/adaptive_pi_error_domain.h) | [error_domain_test.cpp](../../platform/ara-core/tests/error_domain_test.cpp) | 16 |
| AP-R3-CORE-003 | Implemented | [error_code.h](../../platform/ara-core/include/ara/core/error_code.h) | [error_code_test.cpp](../../platform/ara-core/tests/error_code_test.cpp) | 6 |
| AP-R3-CORE-004 | Implemented | [error_code.h](../../platform/ara-core/include/ara/core/error_code.h), [adaptive_pi_error_domain.h](../../platform/ara-core/include/ara/core/adaptive_pi_error_domain.h) | [error_code_comparison_test.cpp](../../platform/ara-core/tests/error_code_comparison_test.cpp) | 21 |
| AP-R3-CORE-005 | Implemented | [result.h](../../platform/ara-core/include/ara/core/result.h) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 46 |
| AP-R3-CORE-006 | Approved | [result.h](../../platform/ara-core/include/ara/core/result.h) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 14 |
| AP-R3-CORE-007 | Implemented | [result.h](../../platform/ara-core/include/ara/core/result.h) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 27 |
| AP-R3-CORE-008 | Implemented | [result.h](../../platform/ara-core/include/ara/core/result.h), [adaptive_pi_error_domain.h](../../platform/ara-core/include/ara/core/adaptive_pi_error_domain.h) | [error_domain_test.cpp](../../platform/ara-core/tests/error_domain_test.cpp), [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 37 |
| AP-R3-CORE-009 | Implemented | [result.h](../../platform/ara-core/include/ara/core/result.h) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 20 |
| AP-R3-CORE-010 | Implemented | [result.h](../../platform/ara-core/include/ara/core/result.h) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp) | 2 |
| AP-R3-CORE-011 | Implemented | [logger_registry.cpp](../../platform/ara-log/src/logger_registry.cpp), [main.cpp](../../apps/hello-adaptive/src/main.cpp) | [result_test.cpp](../../platform/ara-core/tests/result_test.cpp), [initialization_test.cpp](../../apps/hello-adaptive/tests/initialization_test.cpp) | 6 |
| AP-R3-CORE-012 | Implemented | [error_domain.h](../../platform/ara-core/include/ara/core/error_domain.h), [error_code.h](../../platform/ara-core/include/ara/core/error_code.h) | [error_domain_extensibility_test.cpp](../../platform/ara-core/tests/error_domain_extensibility_test.cpp) | 12 |
| AP-R3-LOG-001 | Approved | [log_level.h](../../platform/ara-log/include/ara/log/log_level.h) | [log_level_test.cpp](../../platform/ara-log/tests/log_level_test.cpp) | 7 |
| AP-R3-LOG-002 | Approved | [logger.cpp](../../platform/ara-log/src/logger.cpp), [logger_registry.cpp](../../platform/ara-log/src/logger_registry.cpp) | [logger_context_test.cpp](../../platform/ara-log/tests/logger_context_test.cpp), [logger_threshold_test.cpp](../../platform/ara-log/tests/logger_threshold_test.cpp), [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp), [logging_contract_test.cpp](../../platform/ara-log/tests/logging_contract_test.cpp) | 78 |
| AP-R3-LOG-003 | Approved | [logger_registry.cpp](../../platform/ara-log/src/logger_registry.cpp) | [logger_allocation_failure_test.cpp](../../platform/ara-log/tests/logger_allocation_failure_test.cpp), [logger_ownership_test.cpp](../../platform/ara-log/tests/logger_ownership_test.cpp), [initialization_test.cpp](../../apps/hello-adaptive/tests/initialization_test.cpp) | 8 |
| AP-R3-LOG-004 | Approved | [logger_registry.cpp](../../platform/ara-log/src/logger_registry.cpp), [console_sink.cpp](../../platform/ara-log/src/console_sink.cpp) | [console_sink_test.cpp](../../platform/ara-log/tests/console_sink_test.cpp) | 1 |
| AP-R3-LOG-005 | Approved | [log_stream.cpp](../../platform/ara-log/src/log_stream.cpp), [log_stream.h](../../platform/ara-log/include/ara/log/log_stream.h) | [log_stream_test.cpp](../../platform/ara-log/tests/log_stream_test.cpp), [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp), [logging_contract_test.cpp](../../platform/ara-log/tests/logging_contract_test.cpp) | 45 |
| AP-R3-LOG-006 | Approved | [console_record_formatter.cpp](../../platform/ara-log/src/console_record_formatter.cpp) | [console_timestamp_test.cpp](../../platform/ara-log/tests/console_timestamp_test.cpp), [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp) | 24 |
| AP-R3-LOG-007 | Approved | [linux_metadata_provider.cpp](../../platform/ara-log/src/linux_metadata_provider.cpp), [logger.cpp](../../platform/ara-log/src/logger.cpp) | [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp), [metadata_provider_test.cpp](../../platform/ara-log/tests/metadata_provider_test.cpp) | 10 |
| AP-R3-LOG-008 | Approved | [logger.cpp](../../platform/ara-log/src/logger.cpp), [log_stream.cpp](../../platform/ara-log/src/log_stream.cpp) | [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp), [logging_contract_test.cpp](../../platform/ara-log/tests/logging_contract_test.cpp), [logging_failure_test.cpp](../../platform/ara-log/tests/logging_failure_test.cpp) | 19 |
| AP-R3-LOG-009 | Approved | [console_sink.cpp](../../platform/ara-log/src/console_sink.cpp) | [log_record_test.cpp](../../platform/ara-log/tests/log_record_test.cpp), [logging_contract_test.cpp](../../platform/ara-log/tests/logging_contract_test.cpp) | 5 |
| AP-R3-LOG-010 | Approved | [logger.h](../../platform/ara-log/include/ara/log/logger.h), [modelled_message.h](../../platform/ara-log/include/ara/log/modelled_message.h) | [modelled_message_test.cpp](../../platform/ara-log/tests/modelled_message_test.cpp), [modelled_compile_test.cmake](../../platform/ara-log/tests/modelled_compile_test.cmake) | 19 |
| AP-R3-LOG-011 | Approved | [logger.h](../../platform/ara-log/include/ara/log/logger.h), [trace_routing.h](../../platform/ara-log/include/ara/log/trace_routing.h), [trace_artifact.h](../../platform/ara-log/include/ara/log/trace_artifact.h) | [trace_routing_test.cpp](../../platform/ara-log/tests/trace_routing_test.cpp) | 12 |

## Compile-time checks and repeatable host workflow

Core constexpr identity checks are compiled from error_domain_test.cpp. Result
constructor constraints and configuration-dependent ValueOrThrow presence or
absence are compiled from result_test.cpp. Stream ownership/type checks and
modelled signature/routing checks are in the logging test translation units.
Building their executable targets compiles these assertions; they are not
separate runtime assertions.

The modelled compile-test script is registered in platform/ara-log/tests/CMakeLists.txt
for GCC/Clang. It runs 17 compiler cases: three successful controls and fourteen
expected failures, checking diagnostic categories as well as exit status. It
uses the selected exception mode. CTest therefore executes negative compilation
checks alongside ordinary tests. A successful library build alone does not
instantiate every public logging template; the smoke executable supplies target
compile/link and runtime evidence for Log, LogWith, attributes, and all routes.

```bash
cmake --preset debug-app
cmake --build build/debug-app --parallel 2
cmake --preset debug-unit-tests
cmake --build build/debug-unit-tests --parallel 2
```

The default test build invokes run-unit-tests, an ALL target that depends on the
test executables and runs CTest. A later explicit run uses
`ctest --preset debug-unit-tests`. Provision de_DE.UTF-8 before testing; Host CI
already does this. If generating a locale without modifying system configuration:

```bash
mkdir -p /tmp/adaptive-pi-locales
localedef --no-archive -i de_DE -f UTF-8 /tmp/adaptive-pi-locales/de_DE.UTF-8
LOCPATH=/tmp/adaptive-pi-locales ctest --preset debug-unit-tests
```

[Host CI](../../.github/workflows/host-ci.yml) runs formatting, both native
exception configurations, ESBMC proofs, and the SDK cross-build on PRs. Preserve
the successful job links and tested SHA before closure; a workflow's existence
is not passing evidence. Local application/unit-test builds establish their
observed checks, not completion of the entire remote Host CI workflow.

## Target workflow and observed output

```bash
cmake --preset debug-qemu-app -DADAPTIVE_PI_BUILD_LOG_SMOKE=ON
cmake --build --preset debug-qemu-app
readelf -h build/debug-qemu-app/apps/hello-adaptive/hello-adaptive
readelf -h build/debug-qemu-app/platform/ara-log/ara_log_target_smoke
scripts/qemu/start-development-image.sh
scripts/qemu/deploy-binary.sh build/debug-qemu-app/apps/hello-adaptive/hello-adaptive
scripts/qemu/deploy-binary.sh build/debug-qemu-app/platform/ara-log/ara_log_target_smoke
ssh -o BatchMode=yes -p 2222 root@localhost /usr/local/bin/hello-adaptive
ssh -o BatchMode=yes -p 2222 root@localhost /usr/local/bin/ara_log_target_smoke
scripts/qemu/stop-development-image.sh
```

Observed hello-adaptive stdout (exit 0):

```text
[2026-10-11T00:31:33.487Z][284][284][INFO][HELLO] Application startup
[2026-10-11T00:31:33.490Z][284][284][INFO][HELLO] id=HELLO_GREETING Hello from AdaptivePi on Embedded Linux.
```

Observed ara_log_target_smoke stdout (exit 0, including its two-trace-call check):

```text
[2026-10-11T00:31:33.516Z][283][283][INFO][SMOKE] id=SMOKE 2
[2026-10-11T00:31:33.518Z][283][283][INFO][SMOKE] id=SMOKE 2 location="smoke.cpp:1" tag="target"
```

The QEMU instance started for this audit was stopped afterward. These checks use
SSH-deployed /usr/local/bin executables. They do not verify Yocto packaging or
systemd activation of /usr/bin/hello-adaptive. QEMU provides runtime integration
evidence, not real hardware timing or Raspberry Pi peripheral validation.

## Documentation and clean-room boundary

- [Core API guide](../guides/ara-core.md) and [logging API guide](../guides/ara-log-foundation.md).
- [Core component decisions](../../platform/ara-core/doc/design-decisions.md) and [logging component decisions](../../platform/ara-log/doc/design-decisions.md).
- [Exception policy](../adr/0006-exception-build-policy.md) and [logging contracts](../adr/0007-release-3-logging-contracts.md).

The requirement baseline contains official AUTOSAR R23-11 source traces and
project-owned normative wording. No proprietary AUTOSAR implementation is used,
and no AUTOSAR conformance is claimed. Key deliberate limits are a single
project-owned domain, the reduced Result interface, the two-operation logging
factory, console-only output, fixed record bounds, project-defined formatting,
manual message/routing definitions, and an educational trace artifact boundary.
Generated ARXML artifacts, DLT, runtime trace configuration, communication,
Execution Management, and diagnostics remain outside this release.

## Remaining closure actions

1. Dependency issues #2, #14 (Result), #3, #15, #16, and #4 are confirmed closed.
   Their code is included in the audited revision.
2. PR #69 supplies successful Host CI for the audited code tree as recorded
   above. Run and retain PR checks for this documentation change before merge.
3. Review the requirement evidence and reconcile status deviations using the
   confirmed dual-mode evidence. Preserve normative text and deliberate deviations.
4. README records the completed implementation/verification milestone.
5. Create the release/tag only after every acceptance criterion in #5 is met.

Release publication must still satisfy the final revision checks and issue #5
acceptance criteria.
