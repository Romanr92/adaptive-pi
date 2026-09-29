#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/error_code.h"

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

    static_assert(std::is_enum_v<AdaptivePiErrc>, "AdaptivePiErrc must be an enumeration");

    static_assert(std::is_same_v<std::underlying_type_t<AdaptivePiErrc>, ErrorCode::ValueType>,
                  "AdaptivePiErrc must use ErrorCode::ValueType as its underlying type");

    constexpr ErrorCode compile_time_error{MakeErrorCode(AdaptivePiErrc::kInvalidArgument)};

    static_assert(compile_time_error == AdaptivePiErrc::kInvalidArgument,
                  "ErrorCode-to-enum equality must work in a constant expression");

    static_assert(AdaptivePiErrc::kInvalidState != compile_time_error,
                  "Enum-to-ErrorCode inequality must work in a constant expression");

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that enum conversion preserves the error value and concrete domain.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain>
    {
    };

    /* Verify that enum conversion preserves the error value and concrete domain.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Read the converted error value and domain address.
     * 3. Expect: Both match the enumeration and AdaptivePi domain.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, EnumConversionStoresValueAndDomain)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      /* Act */
      const auto actual_value = error_code.Value();
      const auto actual_domain = &error_code.Domain();

      /* Expect */
      EXPECT_EQ(actual_value, static_cast<ErrorCode::ValueType>(parameter.error));
      EXPECT_EQ(actual_domain, &GetAdaptivePiErrorDomain());
    }

    std::string ErrorCodeComparisonCaseName_EnumConversionStoresValueAndDomain(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons, AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison,
      ::testing::Values(
        ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_EnumConversionStoresValueAndDomain{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_EnumConversionStoresValueAndDomain);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that equivalent error codes compare equal.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_EquivalentErrorCodesCompareEqual
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual>
    {
    };

    /* Verify that equivalent error codes compare equal.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Evaluate equality and inequality for codes with the same value and domain.
     * 3. Expect: Equality is true and inequality is false.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_EquivalentErrorCodesCompareEqual,
           EquivalentErrorCodesCompareEqual)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual& parameter{GetParam()};
      const ErrorCode first{MakeErrorCode(parameter.error)};
      const ErrorCode second{static_cast<ErrorCode::ValueType>(parameter.error), GetAdaptivePiErrorDomain()};

      /* Act */
      const auto equal = first == second;
      const auto unequal = first != second;

      /* Expect */
      EXPECT_TRUE(equal);
      EXPECT_FALSE(unequal);
    }

    std::string ErrorCodeComparisonCaseName_EquivalentErrorCodesCompareEqual(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons, AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_EquivalentErrorCodesCompareEqual,
      ::testing::Values(
        ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_EquivalentErrorCodesCompareEqual{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_EquivalentErrorCodesCompareEqual);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that different values in one domain compare unequal.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentValuesInSameDomainCompareUnequal
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal>
    {
    };

    /* Verify that different values in one domain compare unequal.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Evaluate equality and inequality for codes with different values.
     * 3. Expect: Equality is false and inequality is true.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentValuesInSameDomainCompareUnequal,
           DifferentValuesInSameDomainCompareUnequal)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal& parameter{GetParam()};
      const ErrorCode first{MakeErrorCode(parameter.error)};
      const ErrorCode second{MakeErrorCode(parameter.different_error)};

      /* Act */
      const auto equal = first == second;
      const auto unequal = first != second;

      /* Expect */
      EXPECT_FALSE(equal);
      EXPECT_TRUE(unequal);
    }

    std::string ErrorCodeComparisonCaseName_DifferentValuesInSameDomainCompareUnequal(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons,
      AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentValuesInSameDomainCompareUnequal,
      ::testing::Values(
        ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_DifferentValuesInSameDomainCompareUnequal{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_DifferentValuesInSameDomainCompareUnequal);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that equal values from different domains compare unequal.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_SameValueInDifferentDomainsCompareUnequal
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal>
    {
    };

    /* Verify that equal values from different domains compare unequal.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Evaluate equality and inequality across the two domains.
     * 3. Expect: Equality is false and inequality is true.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_SameValueInDifferentDomainsCompareUnequal,
           SameValueInDifferentDomainsCompareUnequal)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal& parameter{GetParam()};
      const ErrorCode adaptive_pi_error{MakeErrorCode(parameter.error)};
      const ErrorDomain different_domain{parameter.different_domain_id, "DifferentDomain"
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
      const ErrorCode different_domain_error{static_cast<ErrorCode::ValueType>(parameter.error), different_domain};

      /* Act */
      const auto equal = adaptive_pi_error == different_domain_error;
      const auto unequal = adaptive_pi_error != different_domain_error;

      /* Expect */
      EXPECT_FALSE(equal);
      EXPECT_TRUE(unequal);
    }

    std::string ErrorCodeComparisonCaseName_SameValueInDifferentDomainsCompareUnequal(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons,
      AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_SameValueInDifferentDomainsCompareUnequal,
      ::testing::Values(
        ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_SameValueInDifferentDomainsCompareUnequal{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_SameValueInDifferentDomainsCompareUnequal);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that a matching enumeration compares equal in both operand orders.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_MatchingEnumComparesEqualInBothDirections
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections>
    {
    };

    /* Verify that a matching enumeration compares equal in both operand orders.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Evaluate both comparison operators with the enum on either side.
     * 3. Expect: Both equality checks are true and both inequality checks are false.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_MatchingEnumComparesEqualInBothDirections,
           MatchingEnumComparesEqualInBothDirections)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      /* Act */
      const auto code_equals_enum = error_code == parameter.error;
      const auto enum_equals_code = parameter.error == error_code;
      const auto code_differs_from_enum = error_code != parameter.error;
      const auto enum_differs_from_code = parameter.error != error_code;

      /* Expect */
      EXPECT_TRUE(code_equals_enum);
      EXPECT_TRUE(enum_equals_code);
      EXPECT_FALSE(code_differs_from_enum);
      EXPECT_FALSE(enum_differs_from_code);
    }

    std::string ErrorCodeComparisonCaseName_MatchingEnumComparesEqualInBothDirections(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons,
      AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_MatchingEnumComparesEqualInBothDirections,
      ::testing::Values(
        ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_MatchingEnumComparesEqualInBothDirections{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_MatchingEnumComparesEqualInBothDirections);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections
    {
        AdaptivePiErrc error;
        AdaptivePiErrc different_error;
        ErrorDomain::IdType different_domain_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that a different enumeration compares unequal in both operand
     * orders. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentEnumComparesUnequalInBothDirections
        : public ::testing::TestWithParam<ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections>
    {
    };

    /* Verify that a different enumeration compares unequal in both operand orders.
     * 1. Arrange: Read the case inputs and construct the domains and error codes needed for this check.
     * 2. Act: Evaluate both comparison operators with the different enum on either side.
     * 3. Expect: Both equality checks are false and both inequality checks are true.
     */
    TEST_P(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentEnumComparesUnequalInBothDirections,
           DifferentEnumComparesUnequalInBothDirections)
    {
      /* Arrange */
      const ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections& parameter{GetParam()};
      const ErrorCode error_code{MakeErrorCode(parameter.error)};

      /* Act */
      const auto code_equals_enum = error_code == parameter.different_error;
      const auto enum_equals_code = parameter.different_error == error_code;
      const auto code_differs_from_enum = error_code != parameter.different_error;
      const auto enum_differs_from_code = parameter.different_error != error_code;

      /* Expect */
      EXPECT_FALSE(code_equals_enum);
      EXPECT_FALSE(enum_equals_code);
      EXPECT_TRUE(code_differs_from_enum);
      EXPECT_TRUE(enum_differs_from_code);
    }

    std::string ErrorCodeComparisonCaseName_DifferentEnumComparesUnequalInBothDirections(
      const ::testing::TestParamInfo<ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      AdaptivePiErrorComparisons,
      AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison_DifferentEnumComparesUnequalInBothDirections,
      ::testing::Values(
        ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections{
          AdaptivePiErrc::kInvalidArgument, AdaptivePiErrc::kInvalidState, 0x4150490000000002ULL,
          "InvalidArgumentComparisons", "Invalid argument error conversion and comparison behavior"},
        ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections{
          AdaptivePiErrc::kInvalidState, AdaptivePiErrc::kOperationFailed, 0x4150490000000003ULL,
          "InvalidStateComparisons", "Invalid state error conversion and comparison behavior"},
        ErrorCodeComparisonCase_DifferentEnumComparesUnequalInBothDirections{
          AdaptivePiErrc::kOperationFailed, AdaptivePiErrc::kInvalidArgument, 0x4150490000000004ULL,
          "OperationFailedComparisons", "Operation failed error conversion and comparison behavior"}),
      ErrorCodeComparisonCaseName_DifferentEnumComparesUnequalInBothDirections);

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    /* ----------------------------------------------------------------------------------- */

    /* Verify the published AdaptivePi exception preserves both its original error and message.
     * 1. Arrange: Construct the error and wrap it in the project-owned exception type.
     * 2. Act: Retrieve the stored error and textual message from the exception.
     * 3. Expect: The payload and message match the project contract exactly.
     */
    TEST(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, AdaptivePiExceptionPreservesMetadata)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kOperationFailed);
      const AdaptivePiException exception{error};

      /* Act */
      const auto& observed_error = exception.Error();
      const auto* observed_message = exception.what();

      /* Expect */
      EXPECT_EQ(observed_error, error);
      EXPECT_EQ(observed_error.Domain(), GetAdaptivePiErrorDomain());
      EXPECT_STREQ(observed_message, "AdaptivePi operation failed");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify the published exception text is returned exactly as the public contract defines.
     * 1. Arrange: Construct the project-owned exception with the selected error payload.
     * 2. Act: Query the message string from the exception object.
     * 3. Expect: The returned message is the exact published AdaptivePi failure text.
     */
    TEST(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, AdaptivePiExceptionMessageMatchesContract)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidState);
      const AdaptivePiException exception{error};

      /* Act */
      const auto* observed_message = exception.what();

      /* Expect */
      EXPECT_STREQ(observed_message, "AdaptivePi operation failed");
      EXPECT_EQ(exception.Error(), error);
    }

    /* ----------------------------------------------------------------------------------- */
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

    /* Verify the published AdaptivePi domain identity remains stable and self-equal.
     * 1. Arrange: Capture the singleton domain and expected metadata.
     * 2. Act: Read its identifier and name and compare it to itself.
     * 3. Expect: The project-owned identity matches the published contract and is self-equal.
     */
    TEST(AP_R3_CORE_004_ErrorCodeEqualityAndEnumComparison, AdaptivePiDomainIdentityIsStable)
    {
      /* Arrange */
      const auto& domain = GetAdaptivePiErrorDomain();

      /* Act */
      const auto actual_id = domain.Id();
      const auto actual_name = domain.Name();
      const auto equal = domain == GetAdaptivePiErrorDomain();
      const auto unequal = domain != GetAdaptivePiErrorDomain();

      /* Expect */
      EXPECT_EQ(actual_id, 0x4150490000000001ULL);
      EXPECT_EQ(actual_name, "AdaptivePi");
      EXPECT_TRUE(equal);
      EXPECT_FALSE(unequal);
    }

    /* ----------------------------------------------------------------------------------- */

    struct ForeignDomainEnumCase
    {
        AdaptivePiErrc error;
        const char* name;
    };

    std::ostream& operator<<(std::ostream& output, const ForeignDomainEnumCase& parameter)
    {
      return output << parameter.name;
    }

    /* GetParam() supplies each published error number to verify that an equal
     * numeric value in another domain never compares equal to AdaptivePi's enum.
     */
    class AP_R3_CORE_004_ForeignDomainEnumComparison : public ::testing::TestWithParam<ForeignDomainEnumCase>
    {
    };

    /* Verify enum comparisons retain domain identity in both operand orders.
     * 1. Arrange: Store the enum's numeric value in a distinct error domain.
     * 2. Act: Compare that error with the AdaptivePi enum using both operators.
     * 3. Expect: Equality is false and inequality is true in both operand orders.
     */
    TEST_P(AP_R3_CORE_004_ForeignDomainEnumComparison, RejectsMatchingValueFromAnotherDomain)
    {
      /* Arrange */
      const auto& parameter = GetParam();
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      const ErrorDomain other_domain{0x4150490000000002ULL, "OtherDomain", &detail::ThrowAdaptivePiException};
#else
      const ErrorDomain other_domain{0x4150490000000002ULL, "OtherDomain"};
#endif
      const ErrorCode code{static_cast<ErrorCode::ValueType>(parameter.error), other_domain};

      /* Act */
      const bool code_equals_enum = code == parameter.error;
      const bool enum_equals_code = parameter.error == code;
      const bool code_differs_from_enum = code != parameter.error;
      const bool enum_differs_from_code = parameter.error != code;

      /* Expect */
      EXPECT_FALSE(code_equals_enum);
      EXPECT_FALSE(enum_equals_code);
      EXPECT_TRUE(code_differs_from_enum);
      EXPECT_TRUE(enum_differs_from_code);
    }

    std::string ForeignDomainEnumCaseName(const ::testing::TestParamInfo<ForeignDomainEnumCase>& information)
    {
      return information.param.name;
    }

    INSTANTIATE_TEST_SUITE_P(
      PublishedErrors, AP_R3_CORE_004_ForeignDomainEnumComparison,
      ::testing::Values(ForeignDomainEnumCase{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                        ForeignDomainEnumCase{AdaptivePiErrc::kInvalidState, "InvalidState"},
                        ForeignDomainEnumCase{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
      ForeignDomainEnumCaseName);

    /* ======================== End Test_AP_R3_CORE_004 ================================== */
  } /* namespace */
} /* namespace ara::core */