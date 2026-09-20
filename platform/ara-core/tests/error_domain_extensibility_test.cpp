#include "ara/core/adaptive_pi_error_domain.h"

#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>

namespace ara::core
{
  namespace
  {
    /* ========================== Test_AP_R3_CORE_012 ==================================== */

    /* This domain exists only to verify that a new project-owned domain can be
       added without modifying ErrorCode.
       NOLINTNEXTLINE(performance-enum-size) */
    enum class TestServiceErrc : ErrorCode::ValueType
    {
      kUnavailable = 1,
      kInvalidRequest = 2
    };

    // "TST" identifies a test-only namespace and the low-order 1 identifies
    // its first domain. It is deliberately different from the AdaptivePi ID.
    constexpr ErrorDomain kTestServiceErrorDomain{0x5453540000000001ULL, "TestService"};

    [[nodiscard]] constexpr ErrorCode MakeErrorCode(TestServiceErrc error) noexcept
    {
      return ErrorCode{static_cast<ErrorCode::ValueType>(error), kTestServiceErrorDomain};
    }

    constexpr ErrorCode compile_time_test_service_error{MakeErrorCode(TestServiceErrc::kUnavailable)};

    static_assert(kTestServiceErrorDomain != kAdaptivePiErrorDomain,
                  "Additional error domains must use unique identifiers");

    static_assert(compile_time_test_service_error.Domain() == kTestServiceErrorDomain,
                  "ErrorCode must retain a newly added domain");

    static_assert(compile_time_test_service_error.Value() ==
                    static_cast<ErrorCode::ValueType>(TestServiceErrc::kUnavailable),
                  "A new domain conversion must retain its error value");

    struct MultipleDomainCase
    {
        ErrorCode error_code;
        const ErrorDomain* expected_domain;
        ErrorCode::ValueType expected_value;
        ErrorCode same_value_other_domain;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const MultipleDomainCase& parameter)
    {
      return output << parameter.description;
    }

    class AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains : public ::testing::TestWithParam<MultipleDomainCase>
    {
    };

    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains, RetainsValue)
    {
      const MultipleDomainCase& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      EXPECT_EQ(parameter.error_code.Value(), parameter.expected_value);
    }

    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains, RetainsOriginatingDomain)
    {
      const MultipleDomainCase& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      EXPECT_EQ(&parameter.error_code.Domain(), parameter.expected_domain);
    }

    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains, DistinguishesSameValueFromDifferentDomain)
    {
      const MultipleDomainCase& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      EXPECT_NE(parameter.error_code, parameter.same_value_other_domain);
    }

    [[nodiscard]] std::string MultipleDomainTestName(const ::testing::TestParamInfo<MultipleDomainCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ProjectAndAdditionalDomains, AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains,
      ::testing::Values(
        MultipleDomainCase{MakeErrorCode(AdaptivePiErrc::kInvalidArgument), &GetAdaptivePiErrorDomain(),
                           static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidArgument),
                           MakeErrorCode(TestServiceErrc::kUnavailable), "AdaptivePiInvalidArgument",
                           "retains AdaptivePi invalid-argument error and distinguishes another domain"},
        MultipleDomainCase{MakeErrorCode(AdaptivePiErrc::kInvalidState), &GetAdaptivePiErrorDomain(),
                           static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidState),
                           MakeErrorCode(TestServiceErrc::kInvalidRequest), "AdaptivePiInvalidState",
                           "retains AdaptivePi invalid-state error and distinguishes another domain"},
        MultipleDomainCase{MakeErrorCode(TestServiceErrc::kUnavailable), &kTestServiceErrorDomain,
                           static_cast<ErrorCode::ValueType>(TestServiceErrc::kUnavailable),
                           MakeErrorCode(AdaptivePiErrc::kInvalidArgument), "TestServiceUnavailable",
                           "retains test-service unavailable error and distinguishes AdaptivePi"},
        MultipleDomainCase{MakeErrorCode(TestServiceErrc::kInvalidRequest), &kTestServiceErrorDomain,
                           static_cast<ErrorCode::ValueType>(TestServiceErrc::kInvalidRequest),
                           MakeErrorCode(AdaptivePiErrc::kInvalidState), "TestServiceInvalidRequest",
                           "retains test-service invalid-request error and distinguishes AdaptivePi"}),
      MultipleDomainTestName);

    /* ======================== End Test_AP_R3_CORE_012 ================================== */
  } // namespace
} // namespace ara::core