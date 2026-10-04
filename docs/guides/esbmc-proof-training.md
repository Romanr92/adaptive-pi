# Write your first meaningful ESBMC proof

This is a practical lesson for developers who know basic C++ but are new to
formal verification. Work through the steps in order. You will write a property,
choose symbolic inputs, call real code, and check that the proof can detect a bug.
The [ESBMC reference guide](esbmc.md) explains installation, capabilities, and CI.

## 1. Start with one sentence

Write what must always be true before writing C++.

A useful sentence is:

> Constructing an ErrorCode with a value must preserve that value.

This covers the stored-value part of AP-R3-CORE-003 in the
[core requirements](../requirements/release-3-ara-core.md). It does not cover
all ErrorCode behaviour.

Avoid “prove ErrorCode works.” That does not tell a reviewer what to check.

| Too vague | A property you can check |
|---|---|
| The result is correct | A successful result reports `HasValue() == true` |
| The buffer is safe | Every access uses an index smaller than its capacity |
| The validator works | Every input missing a required field is rejected |
| The state machine works | A shutdown request cannot produce the Running state |

The last three are examples for suitable future implementations. Do not create
imaginary production APIs to make a proof possible.

**Your first deliverable:** one sentence, a requirement ID or application
contract, and the function you will call.

## 2. Decide what may vary

A unit test might check the values `0`, `1`, and `-1`. A symbolic input lets
ESBMC consider every value of the selected type within the analysed model.

```cpp
extern std::int32_t nondet_int32();
const auto input = nondet_int32();
```

The declaration intentionally has no C++ function definition. ESBMC supplies a
symbolic value. Do not implement it using `rand()`, and do not initialise the
input to a favourite example value.

Choose inputs that could expose a defect. For the stored-value property, the
error value varies; a fixed valid domain is enough. For domain equality, both
identifiers should vary too. Restricting both to the same identifier would miss
failures involving different domains.

## 3. Write Arrange, Act, Assert

- **Arrange:** choose symbolic inputs and prepare valid initial objects.
- **Act:** call the real implementation once or perform the operation sequence.
- **Assert:** state the promised outcome.

Here is a complete proof of the stored-value property. It uses the repository's
real ErrorCode header. The constructor branch supports both exception modes;
the exception converter is outside this property and is never called.

```cpp
#include "ara/core/error_code.h"

#include <cassert>
#include <cstdint>
#include <exception>

extern std::int32_t nondet_int32();

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
void UnusedConverter(const ara::core::ErrorCode&)
{
  std::terminate();
}
#endif

/* Property: ErrorCode retains the supplied value (AP-R3-CORE-003, in part).
 * Input: every int32_t value. No additional assumption on that value.
 * Environment: a valid, non-empty domain that outlives the ErrorCode.
 * Bounds: unwind 32; timeout 60 s, including library-model loops.
 * Excludes: domain identity, comparisons, and exception conversion.
 */
int main()
{
  /* Arrange */
  const auto input = nondet_int32();
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
  const ara::core::ErrorDomain domain{1, "training", UnusedConverter};
#else
  const ara::core::ErrorDomain domain{1, "training"};
#endif

  /* Act */
  const ara::core::ErrorCode error{input, domain};

  /* Assert */
  assert(error.Value() == input);
}
```

Why is this meaningful? A constructor that accidentally stores zero fails for
nonzero symbolic inputs. The expected answer comes from the contract and the
original input. The harness does not duplicate the constructor's implementation.

This is a teaching example. The existing
[ErrorCode proof](../../platform/ara-core/proofs/proof_error_code.cpp) already
covers this property; you do not need a duplicate registered proof.

## 4. Try the example without changing the suite

Save the complete example above as `/tmp/proof_error_value.cpp`. From the
repository root, install the tool if needed and analyse that scratch file:

```bash
scripts/install-esbmc.sh build/debug-esbmc-proofs/tools/esbmc
build/debug-esbmc-proofs/tools/esbmc/bin/esbmc \
  /tmp/proof_error_value.cpp \
  -Iplatform/ara-core/include \
  --z3 --std c++17 --memory-leak-check --unwind 32 \
  -fno-exceptions -DADAPTIVE_PI_EXCEPTIONS_ENABLED=0
```

Expect `VERIFICATION SUCCESSFUL`. Read the output: confirm the intended file
was parsed and that the property was checked. The local lesson uses exceptions
disabled. Exception-enabled execution belongs to CI under the project policy.

Now replace the last assertion in the **scratch file** with:

```cpp
assert(error.Value() != input);
```

Run the same command. Expect `VERIFICATION FAILED` and a nonzero exit status.
Restore the original assertion and rerun; it should pass again.

This exercise checks that your assertion is active and reachable. It does not
prove that every future assertion will be useful. If the intentionally wrong
assertion passes, stop and inspect the harness before trusting it.

## 5. Understand assumptions before using them

An assertion is a promise to verify. An assumption narrows which cases exist.

```cpp
__ESBMC_assume(length <= capacity);
```

This excludes oversized lengths. It is appropriate only when the caller contract
guarantees that limit. If the function is supposed to reject oversized input,
that assumption hides exactly the failure you wanted to test.

For the ErrorCode lesson, no assumption about `input` is needed. Both negative
and positive values belong in the question.

Avoid these mistakes:

| Mistake | Why it is misleading | Better approach |
|---|---|---|
| Assume the result is correct, then assert it | The answer was assumed | Constrain only justified inputs or environment behaviour |
| Assume `x > 10` and `x < 5` | No execution reaches the assertion | Review constraints together and test reachability |
| Assert `actual == actual` | A wrong result still equals itself | Compare with the contract or original input |
| Copy the production algorithm into an expected-value helper | Both copies may share the same mistake | Use a simpler independent specification |
| Disable an inconvenient memory/unwinding check | A reported problem disappears from the question | Understand the counterexample or justify a different scope |

An unreachable assertion can pass without checking any useful execution. During
review, put `assert(false)` temporarily at the point of interest in a scratch
copy. A failure shows that some allowed execution reaches it. Remove the probe
afterwards.

## 6. Check every valid position without writing a large loop

The application metadata proof provides a second practical pattern. Its contract
says that `ApplicationName()` returns `platform-test-service`.

```cpp
#include "adaptive_pi/platform_test/build_info.hpp"

#include <cassert>
#include <cstddef>

extern std::size_t nondet_size();

/* Property: the application name has the expected length and characters.
 * Input: any size_t index; character access is guarded by the view length.
 * Bounds: unwind 32; timeout 60 s for library-model string scans.
 * Excludes: console output, version metadata, and process startup.
 */
int main()
{
  /* Arrange */
  constexpr char expected[] = "platform-test-service";
  const auto index = nondet_size();

  /* Act */
  const auto actual = adaptive_pi::platform_test::ApplicationName();

  /* Assert */
  assert(actual.size() == sizeof(expected) - 1);
  if (index < actual.size())
  {
    assert(actual[index] == expected[index]);
  }
}
```

The symbolic index covers every valid position. The unconditional length check
matters: without it, an empty returned string would skip the character assertion
and might pass. Out-of-range indices are not dereferenced.

Save this as `/tmp/proof_application_name.cpp`. Unlike the header-only ErrorCode
example, this needs the actual implementation source too:

```bash
build/debug-esbmc-proofs/tools/esbmc/bin/esbmc \
  /tmp/proof_application_name.cpp \
  apps/platform-test-service/src/build_info.cpp \
  -Iapps/platform-test-service/include \
  --z3 --std c++17 --memory-leak-check --unwind 32 \
  -fno-exceptions -DADAPTIVE_PI_EXCEPTIONS_ENABLED=0
```

Exercise: change one character in `expected` in the scratch file. Verification
should fail with an index at which the strings disagree. Restore it and confirm
a pass. The registered [metadata proof](../../apps/platform-test-service/proofs/proof_build_info.cpp)
also checks version metadata.

## 7. Give loops and sequences an honest scope

`--unwind 32` is an execution-expansion bound, not “try 32 input values.” Modelled
library operations can have loops even when your harness does not. Keep unwinding
assertions enabled and explain why the bound fits the property.

For a future state-machine proof, write down both the allowed starting states
and the maximum event-sequence length. Check the invariant after each transition;
a valid final state can hide an invalid intermediate state. A sequence of ten
events does not establish correctness for every possible lifetime.

A timeout means the proof did not finish. It is not success and does not by
itself prove that the application is defective.

## 8. Register a new, useful property

Once a property adds useful coverage, place it beside its component:

```text
platform/<component>/proofs/
  CMakeLists.txt
  README.md
  proof_<property>.cpp
```

The same layout works in `apps/<application>/proofs/`. For an example new
header-only core property, the registration would be:

```cmake
add_esbmc_proof(ara-core-new-property
    SOURCES proof_new_property.cpp
    INCLUDE_DIRECTORIES ../include
    UNWIND 32
    TIMEOUT 60
)
```

Use a unique name. List required implementation `.cpp` files in `SOURCES` and
all header search paths in `INCLUDE_DIRECTORIES`. Add external model files,
fixtures, or other inputs using `DEPENDS` when they are not already covered.
Paths are relative to the component's proof directory. `DEPENDS` may name files
or directories; it does not add compiler include paths or link source files.

CI reuses successful evidence only for matching inputs. Automatically tracked
inputs include the harness, component `src/` and `include/` trees, declared include
directories, the manifest, and shared verification configuration. Undeclared
external inputs cannot be detected reliably. Avoid hidden environment-dependent
behaviour and list every external input needed by the claim.

Run a full local check with reuse disabled:

```bash
cmake --preset debug-esbmc-proofs -DADAPTIVE_PI_ESBMC_CACHE_DIR=
```

Configuration already runs the proofs. After editing a harness, use
`cmake --build --preset debug-esbmc-proofs` to run them again; there is no need
to run both commands consecutively just to obtain the same result twice.

## 9. Write a short evidence note

The component's `proofs/README.md` should answer these questions:

```text
Property: What exact statement is checked?
Trace: Which requirement clause or application contract supports it?
Implementation: Which real functions and source files are analysed?
Inputs: Which values vary symbolically?
Assumptions: Which cases are excluded, and why is that justified?
Bounds: Why this unwind value and, if relevant, sequence/schedule limit?
Dependencies: Which headers, models, and fixtures influence the result?
Configuration: ESBMC version, Z3, exception mode, host/model.
Result: Passed, failed, or inconclusive; revision and reproduction command.
Exclusions: What must not be inferred from this proof?
```

A good claim is “the stored-value property passed for all int32 inputs under
these models.” Avoid “the whole platform is formally verified.” Keep unit tests,
integration tests, target builds, and hardware checks.

## 10. Use CI and full-suite actions correctly

The PR check reports `PASSED` for a freshly executed proof and `REUSED` when
matching successful evidence already exists. Both count toward the gate.
Changed proof inputs, new proofs, or missing evidence require execution.
See [incremental CI](esbmc.md#incremental-ci-and-manual-full-runs) for the exact
inputs and cache limitations.

To force a complete hosted run, use GitHub **Actions** and select one of:

- **ESBMC full proofs (without exceptions)**
- **ESBMC full proofs (with exceptions)**

Choose **Run workflow**, select the branch, and run it. These workflows ignore
result reuse, use Z3, and upload full proof logs. GitHub must have the workflow
files on the default branch before their manual controls are available; see
[GitHub’s manual workflow guide](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow).

Before requesting review, confirm that the property has a clear sentence, the
Act section calls real code, assumptions are justified, a deliberate violation
is detected, dependencies and bounds are documented, and the appropriate CI
check is successful.
