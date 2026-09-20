#include "ara/core/adaptive_pi_error_domain.h"

#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ara::core
{
  namespace
  {
    /* ========================== Test_AP_R3_CORE_004 ==================================== */

    struct ErrorCodeComparisonCase
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorCodeComparisonCase& parameter)
    {
      return output << parameter.description;
    }

    static_assert(std::is_enum_v<AdaptivePiErrc>, "AdaptivePiErrc must be an enumeration");

    static_assert(std::is_same_v<std::underlying_type_t<AdaptivePiErrc>, ErrorCode::ValueType>,
                  "AdaptivePiErrc must use ErrorCode::ValueType as its underlying type");

    constexpr ErrorCode compile_time_error{MakeErrorCode(AdaptivePiErrc::kInvalidArgument)};

    static_assert(compile_time_error == AdaptivePiErrc::kInvalidArgument,
                  "ErrorCode-to-enum equality must work in a constant expression");

    static_assert(AdaptivePiErrc::kInvalidState != compile_time_error,
                  "Enum-to-ErrorCode inequality must work in a constant expression");

    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison : public ::testing::TestWithParam<ErrorCodeComparisonCase>
    {
    };

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, EnumConversionStoresValueAndDomain)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      EXPECT_EQ(error_code.Value(), static_cast<ErrorCode::ValueType>(parameter.error));
      EXPECT_EQ(&error_code.Domain(), &GetAdaptivePiErrorDomain());
    }

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, EquivalentErrorCodesCompareEqual)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode first{MakeErrorCode(parameter.error)};
      const ErrorCode second{static_cast<ErrorCode::ValueType>(parameter.error), GetAdaptivePiErrorDomain()};

      EXPECT_TRUE(first == second);
      EXPECT_FALSE(first != second);
    }

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, DifferentValuesInSameDomainCompareUnequal)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode first{MakeErrorCode(parameter.error)};
      const ErrorCode second{MakeErrorCode(parameter.different_error)};

      EXPECT_FALSE(first == second);
      EXPECT_TRUE(first != second);
    }

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, SameValueInDifferentDomainsCompareUnequal)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode adaptive_pi_error{MakeErrorCode(parameter.error)};
      const ErrorDomain different_domain{parameter.different_domain_id, "DifferentDomain"};
      const ErrorCode different_domain_error{static_cast<ErrorCode::ValueType>(parameter.error), different_domain};

      EXPECT_FALSE(adaptive_pi_error == different_domain_error);
      EXPECT_TRUE(adaptive_pi_error != different_domain_error);
    }

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, MatchingEnumComparesEqualInBothDirections)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      EXPECT_TRUE(error_code == parameter.error);
      EXPECT_TRUE(parameter.error == error_code);
      EXPECT_FALSE(error_code != parameter.error);
      EXPECT_FALSE(parameter.error != error_code);
    }

    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, DifferentEnumComparesUnequalInBothDirections)
    {
      const ErrorCodeComparisonCase& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      EXPECT_FALSE(error_code == parameter.different_error);
      EXPECT_FALSE(parameter.different_error == error_code);
      EXPECT_TRUE(error_code != parameter.different_error);
      EXPECT_TRUE(parameter.different_error != error_code);
    }

    std::string ErrorCodeComparisonCaseName(const ::testing::TestParamInfo<ErrorCodeComparisonCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons, AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison,
      ::testing::Values(ErrorCodeComparisonCase{AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState,
                                                0x4150490000000002ULL, "InvalidArgumentComparisons",
                                                "Invalid argument error conversion and comparison behavior"},
                        ErrorCodeComparisonCase{AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed,
                                                0x4150490000000003ULL, "InvalidStateComparisons",
                                                "Invalid state error conversion and comparison behavior"},
                        ErrorCodeComparisonCase{AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument,
                                                0x4150490000000004ULL, "OperationFailedComparisons",
                                                "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName);

    /* ======================== End Test_AP_R3_CORE_004 ================================== */
  } // namespace
} // namespace ara::core