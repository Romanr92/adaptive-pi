#include "ara/core/error_code.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ara::core
{
  namespace
  {
    /* ========================== Test_AP_R3_CORE_003 ==================================== */

    static_assert(std::is_integral_v<ErrorCode::ValueType>, "ErrorCode value representation must be an integral type");

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeContentCase_StoresIntegralErrorValue
    {
        ErrorCode::ValueType value;
        ErrorDomain::IdType domain_id;
        std::string_view domain_name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorCodeContentCase_StoresIntegralErrorValue& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that ErrorCode retains the supplied integral value.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_003_ErrorCodeStoresValueAndDomain
        : public ::testing::TestWithParam<ErrorCodeContentCase_StoresIntegralErrorValue>
    {
    };

    /* Verify that ErrorCode retains the supplied integral value.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Read Value() from the error code.
     * 3. Expect: The returned value equals the supplied integral value.
     */
    TEST_P(AP_R3_CORE_003_ErrorCodeStoresValueAndDomain, StoresIntegralErrorValue)
    {
      /* Arrange */
      const ErrorCodeContentCase_StoresIntegralErrorValue& parameter{GetParam()};
      const ErrorDomain domain{parameter.domain_id, parameter.domain_name
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
                               ,
                               [](const ErrorCode& error)
                               {
                                 /* This test domain preserves its error in a distinct exception type. */
                                 struct DomainException
                                 {
                                     ErrorCode code;
                                 };
                                 throw DomainException{error};
                               }
#endif
      };
      const ErrorCode error_code{parameter.value, domain};

      /* Act */
      const auto actual_value = error_code.Value();

      /* Expect */
      EXPECT_EQ(actual_value, parameter.value);
    }

    std::string ErrorCodeContentCaseName_StoresIntegralErrorValue(
      const ::testing::TestParamInfo<ErrorCodeContentCase_StoresIntegralErrorValue>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(ErrorCodeContents, AP_R3_CORE_003_ErrorCodeStoresValueAndDomain,
                             ::testing::Values(
                               ErrorCodeContentCase_StoresIntegralErrorValue{
                                 std::numeric_limits<ErrorCode::ValueType>::min(), 0x0000000000000001ULL, "CoreDomain",
                                 "MinimumNegativeValueFromCoreDomain",
                                 "Minimum signed error value originating from the core domain"},
                               ErrorCodeContentCase_StoresIntegralErrorValue{
                                 0, 0x0000000000000100ULL, "CommunicationDomain", "ZeroValueFromCommunicationDomain",
                                 "Zero error value originating from the communication domain"},
                               ErrorCodeContentCase_StoresIntegralErrorValue{
                                 std::numeric_limits<ErrorCode::ValueType>::max(), 0x8000000000000001ULL,
                                 "ExecutionDomain", "MaximumPositiveValueFromExecutionDomain",
                                 "Maximum signed error value originating from the execution domain"}),
                             ErrorCodeContentCaseName_StoresIntegralErrorValue);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeContentCase_ReferencesExactOriginatingDomain
    {
        ErrorCode::ValueType value;
        ErrorDomain::IdType domain_id;
        std::string_view domain_name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeContentCase_ReferencesExactOriginatingDomain& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that ErrorCode retains the exact originating domain object.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_003_ErrorCodeStoresValueAndDomain_ReferencesExactOriginatingDomain
        : public ::testing::TestWithParam<ErrorCodeContentCase_ReferencesExactOriginatingDomain>
    {
    };

    /* Verify that ErrorCode retains the exact originating domain object.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Take the address of the domain returned by Domain().
     * 3. Expect: The returned address identifies the original domain object.
     */
    TEST_P(AP_R3_CORE_003_ErrorCodeStoresValueAndDomain_ReferencesExactOriginatingDomain,
           ReferencesExactOriginatingDomain)
    {
      /* Arrange */
      const ErrorCodeContentCase_ReferencesExactOriginatingDomain& parameter{GetParam()};
      const ErrorDomain domain{parameter.domain_id, parameter.domain_name
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
                               ,
                               [](const ErrorCode& error)
                               {
                                 /* This test domain preserves its error in a distinct exception type. */
                                 struct DomainException
                                 {
                                     ErrorCode code;
                                 };
                                 throw DomainException{error};
                               }
#endif
      };
      const ErrorCode error_code{parameter.value, domain};

      /* Act */
      const auto actual_domain = &error_code.Domain();

      /* Expect */
      EXPECT_EQ(actual_domain, &domain);
    }

    std::string ErrorCodeContentCaseName_ReferencesExactOriginatingDomain(
      const ::testing::TestParamInfo<ErrorCodeContentCase_ReferencesExactOriginatingDomain>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(ErrorCodeContents,
                             AP_R3_CORE_003_ErrorCodeStoresValueAndDomain_ReferencesExactOriginatingDomain,
                             ::testing::Values(
                               ErrorCodeContentCase_ReferencesExactOriginatingDomain{
                                 std::numeric_limits<ErrorCode::ValueType>::min(), 0x0000000000000001ULL, "CoreDomain",
                                 "MinimumNegativeValueFromCoreDomain",
                                 "Minimum signed error value originating from the core domain"},
                               ErrorCodeContentCase_ReferencesExactOriginatingDomain{
                                 0, 0x0000000000000100ULL, "CommunicationDomain", "ZeroValueFromCommunicationDomain",
                                 "Zero error value originating from the communication domain"},
                               ErrorCodeContentCase_ReferencesExactOriginatingDomain{
                                 std::numeric_limits<ErrorCode::ValueType>::max(), 0x8000000000000001ULL,
                                 "ExecutionDomain", "MaximumPositiveValueFromExecutionDomain",
                                 "Maximum signed error value originating from the execution domain"}),
                             ErrorCodeContentCaseName_ReferencesExactOriginatingDomain);

    /* ======================== End Test_AP_R3_CORE_003 ================================== */
  } /* namespace */

} /* namespace ara::core */