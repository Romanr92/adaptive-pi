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

    struct ErrorCodeContentCase
    {
        ErrorCode::ValueType value;
        ErrorDomain::IdType domain_id;
        std::string_view domain_name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorCodeContentCase& parameter)
    {
      return output << parameter.description;
    }

    static_assert(std::is_integral_v<ErrorCode::ValueType>, "ErrorCode value representation must be an integral type");

    class AP_R3_CORE_003_ErrorCodeStoresValueAndDomain : public ::testing::TestWithParam<ErrorCodeContentCase>
    {
    };

    TEST_P(AP_R3_CORE_003_ErrorCodeStoresValueAndDomain, StoresIntegralErrorValue)
    {
      const ErrorCodeContentCase& parameter{GetParam()};
      const ErrorDomain domain{parameter.domain_id, parameter.domain_name};
      const ErrorCode error_code{parameter.value, domain};

      EXPECT_EQ(error_code.Value(), parameter.value);
    }

    TEST_P(AP_R3_CORE_003_ErrorCodeStoresValueAndDomain, ReferencesExactOriginatingDomain)
    {
      const ErrorCodeContentCase& parameter{GetParam()};
      const ErrorDomain domain{parameter.domain_id, parameter.domain_name};
      const ErrorCode error_code{parameter.value, domain};

      EXPECT_EQ(&error_code.Domain(), &domain);
    }

    std::string ErrorCodeContentCaseName(const ::testing::TestParamInfo<ErrorCodeContentCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ErrorCodeContents, AP_R3_CORE_003_ErrorCodeStoresValueAndDomain,
      ::testing::Values(ErrorCodeContentCase{std::numeric_limits<ErrorCode::ValueType>::min(), 0x0000000000000001ULL,
                                             "CoreDomain", "MinimumNegativeValueFromCoreDomain",
                                             "Minimum signed error value originating from the core domain"},
                        ErrorCodeContentCase{0, 0x0000000000000100ULL, "CommunicationDomain",
                                             "ZeroValueFromCommunicationDomain",
                                             "Zero error value originating from the communication domain"},
                        ErrorCodeContentCase{std::numeric_limits<ErrorCode::ValueType>::max(), 0x8000000000000001ULL,
                                             "ExecutionDomain", "MaximumPositiveValueFromExecutionDomain",
                                             "Maximum signed error value originating from the execution domain"}),
      ErrorCodeContentCaseName);

    /* ======================== End Test_AP_R3_CORE_003 ================================== */
  } // namespace

} // namespace ara::core