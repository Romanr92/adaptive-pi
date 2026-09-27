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

    /* "TST" identifies a test-only namespace and the low-order 1 identifies */
    /* its first domain. It is deliberately different from the AdaptivePi ID. */
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

    /* ----------------------------------------------------------------------------------- */

    struct MultipleDomainCase_RetainsValue
    {
        ErrorCode error_code;
        const ErrorDomain* expected_domain;
        ErrorCode::ValueType expected_value;
        ErrorCode same_value_other_domain;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const MultipleDomainCase_RetainsValue& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that an error from either project-owned domain retains its value.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains
        : public ::testing::TestWithParam<MultipleDomainCase_RetainsValue>
    {
    };

    /* Verify that an error from either project-owned domain retains its value.
     * 1. Arrange: Read the error codes and expected domain metadata from this case.
     * 2. Act: Read Value() from the case error code.
     * 3. Expect: The value matches its domain-specific enumeration.
     */
    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains, RetainsValue)
    {
      /* Arrange */
      const MultipleDomainCase_RetainsValue& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      /* Act */
      const auto actual_value = parameter.error_code.Value();

      /* Expect */
      EXPECT_EQ(actual_value, parameter.expected_value);
    }

    [[nodiscard]] std::string
    MultipleDomainTestName_RetainsValue(const ::testing::TestParamInfo<MultipleDomainCase_RetainsValue>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ProjectAndAdditionalDomains, AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains,
      ::testing::Values(
        MultipleDomainCase_RetainsValue{MakeErrorCode(AdaptivePiErrc::kInvalidArgument), &GetAdaptivePiErrorDomain(),
                                        static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidArgument),
                                        MakeErrorCode(TestServiceErrc::kUnavailable), "AdaptivePiInvalidArgument",
                                        "retains AdaptivePi invalid-argument error and distinguishes another domain"},
        MultipleDomainCase_RetainsValue{MakeErrorCode(AdaptivePiErrc::kInvalidState), &GetAdaptivePiErrorDomain(),
                                        static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidState),
                                        MakeErrorCode(TestServiceErrc::kInvalidRequest), "AdaptivePiInvalidState",
                                        "retains AdaptivePi invalid-state error and distinguishes another domain"},
        MultipleDomainCase_RetainsValue{MakeErrorCode(TestServiceErrc::kUnavailable), &kTestServiceErrorDomain,
                                        static_cast<ErrorCode::ValueType>(TestServiceErrc::kUnavailable),
                                        MakeErrorCode(AdaptivePiErrc::kInvalidArgument), "TestServiceUnavailable",
                                        "retains test-service unavailable error and distinguishes AdaptivePi"},
        MultipleDomainCase_RetainsValue{MakeErrorCode(TestServiceErrc::kInvalidRequest), &kTestServiceErrorDomain,
                                        static_cast<ErrorCode::ValueType>(TestServiceErrc::kInvalidRequest),
                                        MakeErrorCode(AdaptivePiErrc::kInvalidState), "TestServiceInvalidRequest",
                                        "retains test-service invalid-request error and distinguishes AdaptivePi"}),
      MultipleDomainTestName_RetainsValue);

    /* ----------------------------------------------------------------------------------- */

    struct MultipleDomainCase_RetainsOriginatingDomain
    {
        ErrorCode error_code;
        const ErrorDomain* expected_domain;
        ErrorCode::ValueType expected_value;
        ErrorCode same_value_other_domain;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const MultipleDomainCase_RetainsOriginatingDomain& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that an error from either project-owned domain retains its
     * originating domain. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_RetainsOriginatingDomain
        : public ::testing::TestWithParam<MultipleDomainCase_RetainsOriginatingDomain>
    {
    };

    /* Verify that an error from either project-owned domain retains its originating domain.
     * 1. Arrange: Read the error codes and expected domain metadata from this case.
     * 2. Act: Read the address returned by Domain().
     * 3. Expect: The address matches the expected domain object.
     */
    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_RetainsOriginatingDomain, RetainsOriginatingDomain)
    {
      /* Arrange */
      const MultipleDomainCase_RetainsOriginatingDomain& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      /* Act */
      const auto actual_domain = &parameter.error_code.Domain();

      /* Expect */
      EXPECT_EQ(actual_domain, parameter.expected_domain);
    }

    [[nodiscard]] std::string MultipleDomainTestName_RetainsOriginatingDomain(
      const ::testing::TestParamInfo<MultipleDomainCase_RetainsOriginatingDomain>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(ProjectAndAdditionalDomains,
                             AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_RetainsOriginatingDomain,
                             ::testing::Values(
                               MultipleDomainCase_RetainsOriginatingDomain{
                                 MakeErrorCode(AdaptivePiErrc::kInvalidArgument), &GetAdaptivePiErrorDomain(),
                                 static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidArgument),
                                 MakeErrorCode(TestServiceErrc::kUnavailable), "AdaptivePiInvalidArgument",
                                 "retains AdaptivePi invalid-argument error and distinguishes another domain"},
                               MultipleDomainCase_RetainsOriginatingDomain{
                                 MakeErrorCode(AdaptivePiErrc::kInvalidState), &GetAdaptivePiErrorDomain(),
                                 static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidState),
                                 MakeErrorCode(TestServiceErrc::kInvalidRequest), "AdaptivePiInvalidState",
                                 "retains AdaptivePi invalid-state error and distinguishes another domain"},
                               MultipleDomainCase_RetainsOriginatingDomain{
                                 MakeErrorCode(TestServiceErrc::kUnavailable), &kTestServiceErrorDomain,
                                 static_cast<ErrorCode::ValueType>(TestServiceErrc::kUnavailable),
                                 MakeErrorCode(AdaptivePiErrc::kInvalidArgument), "TestServiceUnavailable",
                                 "retains test-service unavailable error and distinguishes AdaptivePi"},
                               MultipleDomainCase_RetainsOriginatingDomain{
                                 MakeErrorCode(TestServiceErrc::kInvalidRequest), &kTestServiceErrorDomain,
                                 static_cast<ErrorCode::ValueType>(TestServiceErrc::kInvalidRequest),
                                 MakeErrorCode(AdaptivePiErrc::kInvalidState), "TestServiceInvalidRequest",
                                 "retains test-service invalid-request error and distinguishes AdaptivePi"}),
                             MultipleDomainTestName_RetainsOriginatingDomain);

    /* ----------------------------------------------------------------------------------- */

    struct MultipleDomainCase_DistinguishesSameValueFromDifferentDomain
    {
        ErrorCode error_code;
        const ErrorDomain* expected_domain;
        ErrorCode::ValueType expected_value;
        ErrorCode same_value_other_domain;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const MultipleDomainCase_DistinguishesSameValueFromDifferentDomain& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that the same error value remains distinct across domains.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_DistinguishesSameValueFromDifferentDomain
        : public ::testing::TestWithParam<MultipleDomainCase_DistinguishesSameValueFromDifferentDomain>
    {
    };

    /* Verify that the same error value remains distinct across domains.
     * 1. Arrange: Read the error codes and expected domain metadata from this case.
     * 2. Act: Compare error codes containing the same value from different domains.
     * 3. Expect: The codes compare unequal.
     */
    TEST_P(AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_DistinguishesSameValueFromDifferentDomain,
           DistinguishesSameValueFromDifferentDomain)
    {
      /* Arrange */
      const MultipleDomainCase_DistinguishesSameValueFromDifferentDomain& parameter = GetParam();
      SCOPED_TRACE(parameter.description);

      /* Act */
      const bool equal = parameter.error_code == parameter.same_value_other_domain;

      /* Expect */
      EXPECT_FALSE(equal);
    }

    [[nodiscard]] std::string MultipleDomainTestName_DistinguishesSameValueFromDifferentDomain(
      const ::testing::TestParamInfo<MultipleDomainCase_DistinguishesSameValueFromDifferentDomain>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(ProjectAndAdditionalDomains,
                             AP_R3_CORE_012_ErrorCodeSupportsMultipleDomains_DistinguishesSameValueFromDifferentDomain,
                             ::testing::Values(
                               MultipleDomainCase_DistinguishesSameValueFromDifferentDomain{
                                 MakeErrorCode(AdaptivePiErrc::kInvalidArgument), &GetAdaptivePiErrorDomain(),
                                 static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidArgument),
                                 MakeErrorCode(TestServiceErrc::kUnavailable), "AdaptivePiInvalidArgument",
                                 "retains AdaptivePi invalid-argument error and distinguishes another domain"},
                               MultipleDomainCase_DistinguishesSameValueFromDifferentDomain{
                                 MakeErrorCode(AdaptivePiErrc::kInvalidState), &GetAdaptivePiErrorDomain(),
                                 static_cast<ErrorCode::ValueType>(AdaptivePiErrc::kInvalidState),
                                 MakeErrorCode(TestServiceErrc::kInvalidRequest), "AdaptivePiInvalidState",
                                 "retains AdaptivePi invalid-state error and distinguishes another domain"},
                               MultipleDomainCase_DistinguishesSameValueFromDifferentDomain{
                                 MakeErrorCode(TestServiceErrc::kUnavailable), &kTestServiceErrorDomain,
                                 static_cast<ErrorCode::ValueType>(TestServiceErrc::kUnavailable),
                                 MakeErrorCode(AdaptivePiErrc::kInvalidArgument), "TestServiceUnavailable",
                                 "retains test-service unavailable error and distinguishes AdaptivePi"},
                               MultipleDomainCase_DistinguishesSameValueFromDifferentDomain{
                                 MakeErrorCode(TestServiceErrc::kInvalidRequest), &kTestServiceErrorDomain,
                                 static_cast<ErrorCode::ValueType>(TestServiceErrc::kInvalidRequest),
                                 MakeErrorCode(AdaptivePiErrc::kInvalidState), "TestServiceInvalidRequest",
                                 "retains test-service invalid-request error and distinguishes AdaptivePi"}),
                             MultipleDomainTestName_DistinguishesSameValueFromDifferentDomain);

    /* ======================== End Test_AP_R3_CORE_012 ================================== */
  } /* namespace */
} /* namespace ara::core */