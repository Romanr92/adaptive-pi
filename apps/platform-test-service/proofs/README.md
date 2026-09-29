# Platform test service proofs

`platform-test-service-build-info` analyses the actual `src/build_info.cpp`
implementation together with the harness. It checks the existing application
metadata contract (also covered by `tests/build_info_test.cpp`):
`platform-test-service`, version `0.2.0`. This application-local invariant has
no AUTOSAR requirement ID.

The returned views must have exactly the expected lengths. An unconstrained
symbolic `size_t` index checks the contents at every valid position; branches
restrict accesses to each view's bounds. Pointer and bounds checks remain
active. The proof does not assume the returned contents or lengths are correct.

Bound: `--unwind 32`, sufficient for the character-traits model scanning the
21-character application name and five-character version. Unwinding assertions
remain enabled. Memory-leak checking is enabled. Timeout: 60 seconds.
Excluded: console I/O, the application's main function, OS behaviour, and host
STL correctness. The proof uses ESBMC's standard-library models.

Run from the repository root:

```bash
cmake --preset debug-esbmc-proofs
cmake --build --preset debug-esbmc-proofs
```

The exact command and result are recorded in
`build/debug-esbmc-proofs/esbmc/logs/platform-test-service-build-info.log`.
Baseline: ESBMC 8.5, Linux x86_64, exceptions disabled: **passed**.
