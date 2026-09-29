# Raw source coverage and forced-state tests

Coverage is measured only for the production paths selected by
[`scripts/run-coverage.sh`](../../scripts/run-coverage.sh): C/C++ sources and headers
under `apps/` and `platform/`, excluding their test directories and test filenames.
No branch exclusions, fabricated counters, or source filters were added to obtain
the results below. Both applications have their own `tests/main_test.cmake` smoke
test, checking exit status, stdout and stderr.

## Reproduce both configurations

From the repository root:

```bash
scripts/run-coverage.sh build/coverage-check ON
scripts/run-coverage.sh build/coverage-unit-off OFF
```

The optional second argument defaults to `ON`, preserving the previous script
behavior. Use separate build directories for the two configurations. Each run
refreshes the CMake cache (so compiler changes cannot silently discard coverage
flags), clears old `.gcda` counters and scans only its selected build directory.
The default build runs CTest once through `run-unit-tests`; the script does not
run it a second time. An existing repository-root `site/` directory is left alone.
Reports are written to `<build-directory>/site/coverage/index.html`.

Measured with Clang/LLVM 22.1.8, the host libstdc++, and `--coverage -O0 -g`:

| Configuration | Passing tests | Lines | Functions | Raw branches |
|---|---:|---:|---:|---:|
| Exceptions disabled | 213 | 161/161 (100%) | 290/290 (100%) | 140/140 (100%) |
| Exceptions enabled | 246 | 196/196 (100%) | 334/334 (100%) | 185/206 (89.8%) |

These are results for this source/toolchain combination, not permanent thresholds
or evidence that every possible payload type or requirement has been verified.

## Why the tests deliberately force internal states

The additional white-box checks are **coverage-only fault injection**. Their goal
is to execute defensive branches that ordinary public API use cannot reach, for
each template instantiation already exercised by the suite. They do not establish
that those corrupted states can arise in a valid application.

| Guard | Why the state is infeasible through valid public use | How the test reaches it |
|---|---|---|
| `ResultStorage::index()` finds an empty active optional | Successful construction engages the active slot. Replacement constructs the next payload before changing the active index; failure leaves the original slot engaged. | In a death-test child, reset the active optional and call `HasValue()`. |
| `ResultStorage::get()` finds an empty active optional | The same storage invariant applies to all mutable, const and rvalue getters. | Reset the active optional in the child, then call the exact getter instantiation. |
| Error-storage `get<1>()` finds the value alternative | Public `Error()` terminates at its state check before reaching this internal mismatch. `ThrowIfError()` only selects error storage after testing the state. | Bypass the outer check and select a live value alternative before calling the internal getter. |
| Value-storage `get<0>()` finds the error alternative | This *is* reachable through invalid public `Value()` use, and public death tests cover it. Additional direct probes cover the remaining template/ref-qualified copies of the guard. | Select a live error alternative before calling the internal getter. |

`AP_R3_CORE_007_EmptySlotDeathTest` covers the ordinary payload types. The existing
tracked-payload construction/replacement tests contain separate guard probes so
they exercise their own payload types without depending on another test's support
objects. `AP_R3_CORE_008_{Mutable,Const,Rvalue}StorageGuardDeathTest` checks valid
references as well as both forced failure states. Each typed test has its own type
list and descriptive case names.

Fault injection uses real optional/variant objects and `reset()`/`emplace()`; it
does not reinterpret unconstructed memory or rely on an out-of-bounds index.
Mutations used for death assertions run in the child, leaving the parent's object
intact. The test translation unit uses a concrete allowlist of pointers to private
members, obtained through test-only explicit template instantiations. The
[C++ explicit-instantiation access rule](https://www.eel.is/c++draft/temp.spec.general)
permits naming those members in the instantiation arguments. The accessors expose
only the storage, slot array, active index and exception converter needed by these
probes. They do not redefine `private`/`protected` or alter any production class
or standard-library header, preserving identical class definitions across
translation units.

The test-only runner installs a terminate handler in gcov-instrumented builds. It
calls the runtime's `__gcov_dump()` before delegating to the original terminate
handler, so aborting children contribute their actual execution counters. The
original termination behavior remains in effect. Child counter dumps may include
inherited parent counts; the report is evidence of execution, not an exact
application invocation count.

## Remaining exception-enabled branches

All 21 remaining missing branches are attributed to the initial
`slots_[0].emplace(tag, ...)` call in `ResultStorage` (`result.h`, currently line
34). All explicit source-level conditionals have both outcomes exercised in every
reported instantiation. The missing edges are compiler-generated exception
cleanup paths around construction, not additional `if` decisions in the source.

At `-O0`, calls through `std::optional::emplace` can retain exception cleanup edges
even when the selected payload construction cannot throw. Empty-slot or
wrong-alternative mutation cannot cause a constructor's exception edge to execute:
there is no fully constructed Result available to mutate at that point.

The following inventory accounts for every remaining edge. Counts refer to
constructor instantiations, not lines. Long test-double names are identified by
their owning test below.

| Storage / selected alternative | Missing edges | Reason the selected construction does not throw in these cases |
|---|---:|---|
| Tracked value/error in `ConstructsAndDestroysOnlySelectedAlternative` | 2 | Both alternatives use the test double's `noexcept` move constructor. |
| Void success / tracked error in `ConstructsAnErrorOnlyOnFailure` | 2 | Success constructs `std::monostate`; error uses a `noexcept` move. |
| Value/error in `DirectConstructionSupportsMoveOnlyAlternatives` | 2 | Both test doubles have `noexcept` move constructors. |
| Enum error in `FactoryFunctionsSupportInPlaceConstruction` | 1 | Constructing the enum is nonthrowing; the unselected string-containing value is never constructed. |
| Value in `EmplaceValueAndErrorReplaceActiveAlternative` | 1 | Initial construction uses the tracked value's `noexcept` move. |
| Void success in `PreservesStateOnFailureAndReplacesOnSuccess` | 1 | Only `std::monostate` is constructed, not the potentially throwing error payload. |
| `Result<unique_ptr<int>, ErrorCode>` value, moved/default constructed | 2 | `unique_ptr` move and default construction are `noexcept`. |
| `Result<string, ErrorCode>` error, rvalue/const-reference arguments | 2 | `ErrorCode` copy/move is nonthrowing; no string is constructed. |
| `Result<string, ErrorCode>` value from the existing 3-byte/8-byte character arrays | 2 | The valid short strings fit in this host libstdc++'s local string buffer, so these construction paths do not allocate. This reason is toolchain-specific, not a universal standard-library guarantee. |
| `Result<int, unique_ptr<int>>` and `Result<void, unique_ptr<int>>` error | 2 | The selected `unique_ptr` move is `noexcept`. |
| `Result<string, ErrorCode>` default value | 1 | Default `std::string` construction with the standard allocator is nonthrowing. |
| `Result<int, unique_ptr<int>>` default value | 1 | The selected `int` construction is nonthrowing; no error payload is constructed. |
| `Result<void, unique_ptr<int>>` success | 1 | The selected `std::monostate` construction is nonthrowing. |
| `Result<unique_ptr<int>, ErrorCode>` error from a const reference | 1 | Copying `ErrorCode` is nonthrowing; the value alternative is not constructed. |
| **Total** | **21** | |

Potentially throwing test payloads have explicit initial-failure tests, including
the tracked replacement payloads and the in-place construction payload. The
resource-owning constructor tests exercise failure and successful retry, checking
that both paths release acquired resources. Their exception edges are covered.

An exception escaping a `noexcept` operation terminates rather than unwinding into
this cleanup edge. Forcing undefined behavior, rewriting counters, excluding the
edge, or changing the compiler/optimization policy would not demonstrate execution
of the current raw branch. Therefore the exception-enabled report intentionally
retains these 21 uncovered edges. **100% raw branches is achieved for the default
exception-disabled configuration, not for the exception-enabled configuration.**
