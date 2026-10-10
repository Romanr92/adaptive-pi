#include "ara/core/adaptive_pi_error_domain.h"

#include <cstddef>
#include <cstdlib>
#include <gtest/gtest.h>
#include <new>
#include <ostream>
#include <string>

/* Test-only entry-point renaming exposes the real application implementation. */
#define main HelloAdaptiveMain
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "../src/main.cpp"
#undef main

#if defined(HELLO_ALLOCATION_WRAP)
namespace
{
  thread_local int allocations_until_failure = -1;
  thread_local int injection_hits = 0;

  bool InjectFailure() noexcept
  {
    if (allocations_until_failure < 0)
    {
      return false;
    }
    if (allocations_until_failure-- == 0)
    {
      ++injection_hits;
      return true;
    }
    return false;
  }
} // namespace

/* GNU ld requires these reserved ABI identifiers for allocation wrapping. */
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __real__ZnwmRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept;
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __real__ZnamRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept;
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __wrap__ZnwmRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept
{
  return InjectFailure() ? nullptr : __real__ZnwmRKSt9nothrow_t(size, tag);
}
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __wrap__ZnamRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept
{
  return InjectFailure() ? nullptr : __real__ZnamRKSt9nothrow_t(size, tag);
}
#endif

namespace
{
  /* =============================== Test_AP_R3_CORE_011 =============================== */
  /* ----------------------------------------------------------------------------------- */
  /* Verify that initialization registers the application's configured logger.
   * 1. Arrange: Use an isolated process with no HELLO registration.
   * 2. Act: Initialize and retrieve the registered logger.
   * 3. Expect: Success and the application's exact context properties.
   */
  TEST(AP_R3_CORE_011_HelloInitialization, RegistersApplicationLogger)
  {
    /* Arrange */
    constexpr int c_expected_exit = EXIT_SUCCESS;

    /* Act */
    const auto initialize_and_check = []
    {
      const auto result = hello_adaptive::Initialize();
      if (!result.HasValue())
      {
        std::_Exit(EXIT_FAILURE);
      }
      const auto& logger = ara::log::CreateLogger(hello_adaptive::c_context_id, hello_adaptive::c_context_description,
                                                  hello_adaptive::c_threshold);
      const bool matches = logger.ContextId() == "HELLO" &&
                           logger.ContextDescription() == "AdaptivePi hello application" &&
                           logger.DefaultThreshold() == ara::log::LogLevel::kInfo;
      std::_Exit(matches ? EXIT_SUCCESS : EXIT_FAILURE);
    };

    /* Expect */
    EXPECT_EXIT(initialize_and_check(), testing::ExitedWithCode(c_expected_exit), "");
  }

#if defined(HELLO_ALLOCATION_WRAP)
  /* ----------------------------------------------------------------------------------- */
  struct InitializationFailureCase
  {
      const char* name;     // Case description
      int allocation_index; // Input registration allocation to fail
  };

  void operator<<(std::ostream& output, const InitializationFailureCase& value)
  {
    output << value.name;
  }

  /* Each case selects a registration allocation; GetParam supplies its index. */
  class AP_R3_CORE_011_HelloInitializationFailure : public testing::TestWithParam<InitializationFailureCase>
  {
  };

  /* Verify that initialization propagates resource failure and permits retry.
   * 1. Arrange: Select a registration allocation in an isolated process.
   * 2. Act: Fail initialization, then retry after disabling allocation injection.
   * 3. Expect: OperationFailed is returned and the retry succeeds.
   */
  TEST_P(AP_R3_CORE_011_HelloInitializationFailure, ReturnsErrorAndAllowsRetry)
  {
    /* Arrange */
    const auto parameter = GetParam();

    /* Act */
    const auto fail_and_retry = [parameter]
    {
      allocations_until_failure = parameter.allocation_index;
      const auto failed = hello_adaptive::Initialize();
      allocations_until_failure = -1;
      if (injection_hits != 1 || failed.HasValue() || failed.Error() != ara::core::AdaptivePiErrc::kOperationFailed)
      {
        std::_Exit(EXIT_FAILURE);
      }
      const auto retry = hello_adaptive::Initialize();
      std::_Exit(retry.HasValue() ? EXIT_SUCCESS : EXIT_FAILURE);
    };

    /* Expect */
    EXPECT_EXIT(fail_and_retry(), testing::ExitedWithCode(EXIT_SUCCESS), "");
  }

  std::string InitializationFailureName(const testing::TestParamInfo<InitializationFailureCase>& info)
  {
    return info.param.name;
  }

  INSTANTIATE_TEST_SUITE_P(AllocationStages, AP_R3_CORE_011_HelloInitializationFailure,
                           testing::Values(InitializationFailureCase{"ContextId", 0},
                                           InitializationFailureCase{"Description", 1},
                                           InitializationFailureCase{"LoggerObject", 2}),
                           InitializationFailureName);
#endif
  /* ============================= End Test_AP_R3_CORE_011 ============================= */

#if defined(HELLO_ALLOCATION_WRAP)
  /* =============================== Test_AP_R3_LOG_003 ================================ */
  /* ----------------------------------------------------------------------------------- */
  /* Verify the application exits cleanly when mandatory logger creation fails.
   * 1. Arrange: Fail the first registration allocation in an isolated process.
   * 2. Act: Run the actual application entry point.
   * 3. Expect: Failure exit status without termination from missing logger lookup.
   */
  TEST(AP_R3_LOG_003_HelloStartupFailure, ReturnsFailureExitStatus)
  {
    /* Arrange */
    constexpr int c_expected_exit = EXIT_SUCCESS;

    /* Act */
    const auto run_failed_startup = []
    {
      allocations_until_failure = 0;
      const int status = HelloAdaptiveMain();
      allocations_until_failure = -1;
      std::_Exit(injection_hits == 1 && status == EXIT_FAILURE ? EXIT_SUCCESS : EXIT_FAILURE);
    };

    /* Expect */
    EXPECT_EXIT(run_failed_startup(), testing::ExitedWithCode(c_expected_exit), "");
  }
/* ============================= End Test_AP_R3_LOG_003 ============================== */
#endif
} // namespace
