#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/log/logger.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <new>
#include <ostream>
#include <string>

namespace
{
  /* Injection is active only during a factory call on the test thread. */
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

/* GNU linker wrappers intercept only the library's scalar/array nothrow allocations.
 * Forwarding successful calls preserves the standard allocator/deallocator pairing.
 */
// GNU ld requires this reserved identifier for --wrap.
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __real__ZnwmRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept;
// GNU ld requires this reserved identifier for --wrap.
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __real__ZnamRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept;
// GNU ld requires this reserved identifier for --wrap.
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __wrap__ZnwmRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept
{
  return InjectFailure() ? nullptr : __real__ZnwmRKSt9nothrow_t(size, tag);
}
// GNU ld requires this reserved identifier for --wrap.
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void* __wrap__ZnamRKSt9nothrow_t(std::size_t size, const std::nothrow_t& tag) noexcept
{
  return InjectFailure() ? nullptr : __real__ZnamRKSt9nothrow_t(size, tag);
}

namespace
{
  /* ================================ Test_AP_R3_LOG_003 =============================== */

  struct AllocationCase
  {
      const char* name;
      const char* id;
      int failure_index;
  };
  void operator<<(std::ostream& out, const AllocationCase& value)
  {
    out << value.name;
  }
  /* Parameterization exercises failure at each registration allocation stage.
   * GetParam() supplies the current AllocationCase, including the unique context ID
   * and failure index selecting the ID-buffer, description-buffer, or Logger allocation.
   */
  class AP_R3_LOG_003_CreationFailureReturnsError : public testing::TestWithParam<AllocationCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */

  /* Verify failure reporting and absence of partial publication at every allocation stage.
   * 1. Arrange: Select the allocation stage and a unique context ID.
   * 2. Act: Inject one failure, disable injection, then retry with different context data.
   * 3. Expect: Failure reports OperationFailed, failed lookup terminates, and retry succeeds.
   */
  TEST_P(AP_R3_LOG_003_CreationFailureReturnsError, LeavesRegistryUsable)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    injection_hits = 0;
    /* Act */
    allocations_until_failure = parameter.failure_index;
    auto failed = ara::log::TryCreateLogger(parameter.id, "Failed attempt", ara::log::LogLevel::kInfo);
    allocations_until_failure = -1;
    const int hits = injection_hits;
    const auto failed_lookup = [&parameter]
    {
      (void)ara::log::CreateLogger(parameter.id, "Failed attempt", ara::log::LogLevel::kInfo);
    };
    /* Expect */
    EXPECT_EQ(hits, 1);
    ASSERT_FALSE(failed.HasValue());
    EXPECT_EQ(failed.Error(), ara::core::AdaptivePiErrc::kOperationFailed);
    EXPECT_DEATH(failed_lookup(), "");
    auto retry = ara::log::TryCreateLogger(parameter.id, "Successful retry", ara::log::LogLevel::kDebug);
    ASSERT_TRUE(retry.HasValue());
    auto& retrieved = ara::log::CreateLogger(parameter.id, "Successful retry", ara::log::LogLevel::kDebug);
    EXPECT_EQ(&retrieved, &retry.Value().get());
    EXPECT_EQ(retrieved.ContextDescription(), "Successful retry");
  }
  std::string AllocationCaseName(const testing::TestParamInfo<AllocationCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(AllocationStages, AP_R3_LOG_003_CreationFailureReturnsError,
                           testing::Values(AllocationCase{"IdBuffer", "A30", 0},
                                           AllocationCase{"DescriptionBuffer", "A31", 1},
                                           AllocationCase{"LoggerObject", "A32", 2}),
                           AllocationCaseName);
  /* ============================== End Test_AP_R3_LOG_003 ============================= */
} // namespace
