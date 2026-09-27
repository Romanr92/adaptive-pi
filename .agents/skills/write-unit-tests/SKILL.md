---
name: write-unit-tests
description: Write, extend, or refactor AdaptivePi unit tests with requirement traceability and a preference for parameterized tests. Use when the user asks for unit-test code or changes to existing unit tests.
---

# Write AdaptivePi unit tests

LLMs and AI agents are allowed to write and modify unit tests directly. This is an exception to the repository restriction on writing implementation code; it does not authorize changes to production code.

## Requirement coverage

- Read `docs/requirements/README.md` and the relevant requirement entries before writing requirement-based tests. Use the approved behavior, verification method, and stated deviations as the baseline.
- A requirement verified by unit tests may have multiple unit tests. Cover its distinct behaviors, boundary conditions, and failure cases as needed; do not force one test per requirement.
- Keep each test traceable to its requirement ID using the repository's existing naming and grouping conventions.

## Prefer parameterized tests

- Prefer parameterized tests over standalone tests when several cases share setup, execution, and assertions and differ mainly in inputs or expected results. Follow the existing GoogleTest conventions.
- Give parameter cases descriptive names and useful failure diagnostics.
- Use standalone tests only when parameterization adds no meaningful value. For example, just two simple cases may be clearer as standalone tests if a parameterized-test class and case table would add unnecessary complexity. Two cases are an example, not a mandatory cutoff.
- Keep distinct behaviors in separate tests when combining them would require case-dependent branches or obscure the assertions. Multiple parameterized tests may cover different aspects of the same requirement.

## Requirement blocks and test layout

All fixture, requirement, and between-test delimiter comments shall be exactly 89 characters long, measured from `/` to `/` and excluding indentation. Pad labeled delimiters with `=` and unlabeled test separators with `-` to match the requirement delimiter examples below.

For this project's layout, the Fixtures section is reserved for lifecycle setup and cleanup that runs before or after each test, such as `SetUp()` and `TearDown()`. Parameter case structures, printers, and classes that only provide `TestWithParam` parameter access are test-specific support, not lifecycle fixtures for this layout.

Declare lifecycle fixtures at the beginning of the test file, after includes and namespace openings and before requirement test blocks. Enclose them and their necessary supporting declarations between `/* ==================================== Fixtures ===================================== */` and `/* ================================== End Fixtures =================================== */`. Immediately before each lifecycle fixture, add a block comment explaining why it is needed, what its setup prepares, and what its cleanup releases or resets. Omit the Fixtures section when no lifecycle fixtures are needed.

Each parameterized test (`TEST_P` declaration) shall have its own uniquely named parameter structure, `operator<<` printer, `TestWithParam` class, case-name generator, and instantiation. Do not share these between separate tests, even within one requirement. Keep this support beside its test inside the requirement block, with the structure and printer before the parameterized-test class. Document the class immediately above its declaration, explaining why the case data is needed and how `GetParam()` supplies it. Standalone tests do not need artificial parameter structures or parameterized-test classes.

Group tests by requirement using the exact delimiter style below, substituting the relevant requirement ID:

```cpp
/* ==================================== Fixtures ===================================== */

/* Lifecycle fixtures, if needed, with comments explaining setup and cleanup. */

/* ================================== End Fixtures =================================== */

/* ========================== Test_AP_R3_CORE_003 ==================================== */

/* Requirement-specific compile-time checks. */

/* ----------------------------------------------------------------------------------- */

/* First test: its own case structure, printer, documented TestWithParam class,
 * documented TEST_P body, case-name generator, and instantiation. */

/* ----------------------------------------------------------------------------------- */

/* Additional test for the same requirement, with its own complete support. */

/* ======================== End Test_AP_R3_CORE_003 ================================== */
```

- Separate tests for the same requirement with `/* ----------------------------------------------------------------------------------- */`.
- Place any required non-parameterized tests (`TEST` or `TEST_F`) inside the same requirement start/end delimiters as its parameterized tests. Separate them with `/* ----------------------------------------------------------------------------------- */`, just like all other tests in that requirement block.
- When moving to another requirement, close the current block with its `End Test_...` delimiter and open a new `Test_...` block for the next requirement. Keep the requirement ID in fixture or suite names as in `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain`.
- Make each parameterized test self-contained: include its own parameter structure, descriptive case names and descriptions, printer, parameterized-test class, test body, case-name generator, and instantiation with all required inputs and expected values.
- Write each test's setup, execution, assertions, and any test-step printing explicitly in that test. Do not hide these details in shared helpers merely to remove duplication.
- Duplicate test-specific structures, printers, parameterized-test classes, and test steps rather than sharing them between separate tests. Do not make one requirement's tests depend on another requirement's test infrastructure. Share only genuine lifecycle setup or cleanup through the Fixtures section when needed.

## Document every test and its three steps

- Use C-style block comments (`/* comment */`) for test documentation and comments inside tests, including the Arrange, Act, and Expect markers. Use the same block-comment style for multiline documentation.
- Immediately before every test declaration (`TEST`, `TEST_F`, `TEST_P`, or another test-declaration macro), write a clear comment explaining what the test aims to achieve and describing its steps. Place this comment after the dashed separator, directly above the declaration.
- Document exactly three steps in this order: **Arrange**, **Act**, **Expect**. Describe the concrete setup, operation under test, and expected outcome; do not use generic labels alone.
- Inside every test body, use the same three clearly marked comment sections, in the same order: `/* Arrange */`, `/* Act */`, `/* Expect */`. Put setup in Arrange, the operation under test in Act, and assertions in Expect. Each comment must be present at the beginning of its corresponding section, immediately before that section's first statement. The documentation above the test declaration does not replace these comments inside the body.

For example, using this test's own parameter structure and parameterized-test class declared immediately beforehand inside its requirement block:

```cpp
/* ----------------------------------------------------------------------------------- */

/* Verify that ErrorCode preserves the supplied integral error value.
 * 1. Arrange: Read the case parameters and construct the originating domain.
 * 2. Act: Construct an ErrorCode with the supplied value and domain.
 * 3. Expect: The stored value equals the supplied value.
 */
TEST_P(AP_R3_CORE_003_ErrorCodeStoresValueAndDomain, StoresIntegralErrorValue)
{
  /* Arrange */
  const ErrorCodeContentCase& parameter{GetParam()};
  const ErrorDomain domain{parameter.domain_id, parameter.domain_name};

  /* Act */
  const ErrorCode error_code{parameter.value, domain};

  /* Expect */
  EXPECT_EQ(error_code.Value(), parameter.value);
}
```

## Update requirement test traceability

After writing or updating requirement-based unit tests:

- Add or update a separate `- Unit Tests:` entry in each corresponding requirement. List only the `SuiteName.TestName` argument from each test declaration, without generated parameter-case suffix. Keep the names on the same line as `- Unit Tests:`, separated by `; ` (semicolon and space), rather than giving each test its own line or bullet. List each distinct name once, even if several suites use it. Leave the entry empty when no unit tests exist for that requirement.
- Keep the planned tests and verification methods in `Verification`. Do not append the unit-test inventory there.
- Use the requirement's explicit `Status` field to decide whether verification markers apply. For `Status: Implemented`, check every test or suite named in `Verification` against actual test declarations: add `✅` immediately after an existing name and `❌` immediately after a missing name. Match exact test names; for a suite name, require at least one test declared in that exact suite. A fixture declaration alone or a similarly named suite is insufficient.
- For requirements not yet marked `Implemented`, use no verification markers, even if some tests already exist. Remove stale markers when updating entries; do not infer or change implementation status from test existence.
- These markers record existence or absence in the test code, not a passing test run or complete requirement coverage. Report test execution separately.
- Preserve the normative requirement, status, and deviations. Limit requirement edits to the test inventory and verification existence markers.

Example for an implemented requirement:

```markdown
- Status: Implemented
- Verification: Unit test `AP_R3_CORE_003_ErrorCodeStoresValueAndDomain` ✅.
- Unit Tests: `StoresIntegralErrorValue`; `ReferencesExactOriginatingDomain`
```

## Verify the changes

Follow the existing test formatting and CMake structure, and run the affected test target. Report what was verified and any checks that could not run. If a test exposes an implementation defect, explain the defect and guide the user through the production fix without editing production code.
