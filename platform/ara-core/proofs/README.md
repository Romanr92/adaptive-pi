# ara::core proofs

`ara-core-error-code` checks the actual `ErrorCode` and `ErrorDomain` headers.
It supplements AP-R3-CORE-003 (stored value and originating domain reference)
and the ErrorCode-to-ErrorCode part of AP-R3-CORE-004 (equality and inequality).
It does not replace the requirements' unit-test verification.

Both signed 32-bit values and both unsigned 64-bit identifiers are unconstrained
symbolic inputs. The two domain objects have distinct, fixed non-empty names
and remain alive for the entire proof. Equal IDs are deliberately allowed to
check identity-based equality across distinct domain objects. No assumption
restricts the values or IDs to a small enumeration.

Bound: `--unwind 32`, including loops in ESBMC's string-view/character-traits
models. The longest name has six characters. Unwinding assertions, pointer and
bounds checks remain enabled; memory-leak checking is explicitly enabled.
Timeout: 60 seconds. References returned by `Domain()` are bound to named
references before their addresses are compared; ESBMC 8.5 reported a spurious
failure when the equivalent address-of-call expression was used directly.

Excluded: domain-specific enumeration conversion, invalid/empty domain names,
exception conversion, Result, logging, and lifetime violations by callers.
The exception-enabled constructor branch supplies an unused terminating
converter, but local proof evidence covers exceptions disabled only.
The proof uses ESBMC's standard-library models, not a proof of the host STL.

Run from the repository root:

```bash
cmake --preset debug-esbmc-proofs
cmake --build --preset debug-esbmc-proofs
```

The exact command and result are recorded in
`build/debug-esbmc-proofs/esbmc/logs/ara-core-error-code.log`.
Baseline: ESBMC 8.5 with explicit `--z3` (Z3 v4.13.3), Linux x86_64,
exceptions disabled: **passed**. Exception-enabled execution is a separate
CI check; local evidence does not establish that result.
