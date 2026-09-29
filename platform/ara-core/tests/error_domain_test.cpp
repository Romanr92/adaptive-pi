#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/error_code.h"
#include "ara/core/error_domain.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>

namespace ara::core
{
  namespace
  {
    /* ========================== Test_AP_R3_CORE_001 ==================================== */

    /* ----------------------------------------------------------------------------------- */

    struct DomainCase_ReturnsConfiguredIdentifier
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const DomainCase_ReturnsConfiguredIdentifier& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that a domain returns its configured identifier.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_001_ErrorDomainProvidesErrorContext
        : public ::testing::TestWithParam<DomainCase_ReturnsConfiguredIdentifier>
    {
    };

    /* Verify that a domain returns its configured identifier.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Read Id() from the configured domain.
     * 3. Expect: The returned identifier equals the case identifier.
     */
    TEST_P(AP_R3_CORE_001_ErrorDomainProvidesErrorContext, ReturnsConfiguredIdentifier)
    {
      /* Arrange */
      const DomainCase_ReturnsConfiguredIdentifier& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name
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

      /* Act */
      const auto actual_id = domain.Id();

      /* Expect */
      EXPECT_EQ(actual_id, parameter.id);
    }

    std::string DomainCaseName_ReturnsConfiguredIdentifier(
      const ::testing::TestParamInfo<DomainCase_ReturnsConfiguredIdentifier>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ProjectDomainContexts, AP_R3_CORE_001_ErrorDomainProvidesErrorContext,
      ::testing::Values(DomainCase_ReturnsConfiguredIdentifier{0x0000000000000001ULL, "CoreDomain",
                                                               "CoreDomainErrorContext", "Core domain error context"},
                        DomainCase_ReturnsConfiguredIdentifier{0x0000000000000100ULL, "CommunicationDomain",
                                                               "CommunicationDomainErrorContext",
                                                               "Communication domain error context"},
                        DomainCase_ReturnsConfiguredIdentifier{
                          0x8000000000000001ULL, "ExecutionDomain", "ExecutionDomainHighUnsignedIdentifierContext",
                          "Execution domain error context with a high unsigned identifier"}),
      DomainCaseName_ReturnsConfiguredIdentifier);

    /* ----------------------------------------------------------------------------------- */

    struct DomainCase_ReturnsConfiguredName
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const DomainCase_ReturnsConfiguredName& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that a domain returns its configured name.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_001_ErrorDomainProvidesErrorContext_ReturnsConfiguredName
        : public ::testing::TestWithParam<DomainCase_ReturnsConfiguredName>
    {
    };

    /* Verify that a domain returns its configured name.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Read Name() from the configured domain.
     * 3. Expect: The returned name equals the case name.
     */
    TEST_P(AP_R3_CORE_001_ErrorDomainProvidesErrorContext_ReturnsConfiguredName, ReturnsConfiguredName)
    {
      /* Arrange */
      const DomainCase_ReturnsConfiguredName& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name
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

      /* Act */
      const auto actual_name = domain.Name();

      /* Expect */
      EXPECT_EQ(actual_name, parameter.name);
    }

    std::string
    DomainCaseName_ReturnsConfiguredName(const ::testing::TestParamInfo<DomainCase_ReturnsConfiguredName>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ProjectDomainContexts, AP_R3_CORE_001_ErrorDomainProvidesErrorContext_ReturnsConfiguredName,
      ::testing::Values(DomainCase_ReturnsConfiguredName{0x0000000000000001ULL, "CoreDomain", "CoreDomainErrorContext",
                                                         "Core domain error context"},
                        DomainCase_ReturnsConfiguredName{0x0000000000000100ULL, "CommunicationDomain",
                                                         "CommunicationDomainErrorContext",
                                                         "Communication domain error context"},
                        DomainCase_ReturnsConfiguredName{
                          0x8000000000000001ULL, "ExecutionDomain", "ExecutionDomainHighUnsignedIdentifierContext",
                          "Execution domain error context with a high unsigned identifier"}),
      DomainCaseName_ReturnsConfiguredName);

    /* ======================== End Test_AP_R3_CORE_001 ================================== */

    /* ========================== Test_AP_R3_CORE_002 ==================================== */

    constexpr ErrorDomain compile_time_domain{0x0000000000000200ULL, "CompileTimeDomain"
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

    constexpr ErrorDomain same_compile_time_identity{0x0000000000000200ULL, "SameIdentityDifferentName"
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
                                                     ,
                                                     [](const ErrorCode& error)
                                                     {
                                                       /* This test domain preserves its error in a distinct exception
                                                        * type. */
                                                       struct DomainException
                                                       {
                                                           ErrorCode code;
                                                       };
                                                       throw DomainException{error};
                                                     }
#endif
    };

    constexpr ErrorDomain different_compile_time_identity{0x0000000000000201ULL, "DifferentIdentity"
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
                                                          ,
                                                          [](const ErrorCode& error)
                                                          {
                                                            /* This test domain preserves its error in a distinct
                                                             * exception type. */
                                                            struct DomainException
                                                            {
                                                                ErrorCode code;
                                                            };
                                                            throw DomainException{error};
                                                          }
#endif
    };

    static_assert(compile_time_domain.Id() == 0x0000000000000200ULL,
                  "ErrorDomain identifier access must work in a constant expression");

    static_assert(!compile_time_domain.Name().empty(), "A compile-time ErrorDomain must have a non-empty name");

    static_assert(compile_time_domain == same_compile_time_identity,
                  "Domains with the same identifier must compare equal at compile time");

    static_assert(compile_time_domain != different_compile_time_identity,
                  "Domains with different identifiers must compare unequal at compile time");

    /* ----------------------------------------------------------------------------------- */

    struct ErrorDomainIdentityCase_PreservesConfiguredIdentifier
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorDomainIdentityCase_PreservesConfiguredIdentifier& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that reconstructing a domain preserves its identifier.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName
        : public ::testing::TestWithParam<ErrorDomainIdentityCase_PreservesConfiguredIdentifier>
    {
    };

    /* Verify that reconstructing a domain preserves its identifier.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Read the identifiers of the original and reconstructed domains.
     * 3. Expect: Both domains retain the configured identifier.
     */
    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, PreservesConfiguredIdentifier)
    {
      /* Arrange */
      const ErrorDomainIdentityCase_PreservesConfiguredIdentifier& parameter{GetParam()};
      const ErrorDomain first_domain{parameter.id, parameter.name
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
      const ErrorDomain reconstructed_domain{parameter.id, parameter.name
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

      /* Act */
      const auto first_id = first_domain.Id();
      const auto reconstructed_id = reconstructed_domain.Id();

      /* Expect */
      EXPECT_EQ(first_id, parameter.id);
      EXPECT_EQ(reconstructed_id, first_id);
    }

    std::string ErrorDomainIdentityCaseName_PreservesConfiguredIdentifier(
      const ::testing::TestParamInfo<ErrorDomainIdentityCase_PreservesConfiguredIdentifier>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(StableDomainIdentities, AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName,
                             ::testing::Values(
                               ErrorDomainIdentityCase_PreservesConfiguredIdentifier{
                                 0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain", 0x0000000000000002ULL,
                                 "CoreDomainStableIdentity", "Core domain has a stable identity and non-empty name"},
                               ErrorDomainIdentityCase_PreservesConfiguredIdentifier{
                                 0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain",
                                 0x0000000000000101ULL, "CommunicationDomainStableIdentity",
                                 "Communication domain has a stable identity and non-empty name"},
                               ErrorDomainIdentityCase_PreservesConfiguredIdentifier{
                                 0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain",
                                 0x8000000000000002ULL, "ExecutionDomainStableHighUnsignedIdentity",
                                 "Execution domain has a stable high unsigned identity and non-empty name"}),
                             ErrorDomainIdentityCaseName_PreservesConfiguredIdentifier);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorDomainIdentityCase_ProvidesNonEmptyName
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorDomainIdentityCase_ProvidesNonEmptyName& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that a configured domain name is non-empty.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_ProvidesNonEmptyName
        : public ::testing::TestWithParam<ErrorDomainIdentityCase_ProvidesNonEmptyName>
    {
    };

    /* Verify that a configured domain name is non-empty.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Check whether Name() is empty.
     * 3. Expect: The name is not empty.
     */
    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_ProvidesNonEmptyName, ProvidesNonEmptyName)
    {
      /* Arrange */
      const ErrorDomainIdentityCase_ProvidesNonEmptyName& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name
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

      /* Act */
      const auto name_is_empty = domain.Name().empty();

      /* Expect */
      EXPECT_FALSE(name_is_empty);
    }

    std::string ErrorDomainIdentityCaseName_ProvidesNonEmptyName(
      const ::testing::TestParamInfo<ErrorDomainIdentityCase_ProvidesNonEmptyName>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      StableDomainIdentities, AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_ProvidesNonEmptyName,
      ::testing::Values(
        ErrorDomainIdentityCase_ProvidesNonEmptyName{0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain",
                                                     0x0000000000000002ULL, "CoreDomainStableIdentity",
                                                     "Core domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_ProvidesNonEmptyName{
          0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain", 0x0000000000000101ULL,
          "CommunicationDomainStableIdentity", "Communication domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_ProvidesNonEmptyName{
          0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain", 0x8000000000000002ULL,
          "ExecutionDomainStableHighUnsignedIdentity",
          "Execution domain has a stable high unsigned identity and non-empty name"}),
      ErrorDomainIdentityCaseName_ProvidesNonEmptyName);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorDomainIdentityCase_SameIdentifiersCompareEqual
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorDomainIdentityCase_SameIdentifiersCompareEqual& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that equal domain identifiers establish equality regardless of
     * name. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_SameIdentifiersCompareEqual
        : public ::testing::TestWithParam<ErrorDomainIdentityCase_SameIdentifiersCompareEqual>
    {
    };

    /* Verify that equal domain identifiers establish equality regardless of name.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Evaluate equality and inequality for domains with the same identifier.
     * 3. Expect: Equality is true and inequality is false.
     */
    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_SameIdentifiersCompareEqual, SameIdentifiersCompareEqual)
    {
      /* Arrange */
      const ErrorDomainIdentityCase_SameIdentifiersCompareEqual& parameters{GetParam()};
      const ErrorDomain domain{parameters.id, parameters.name
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
      const ErrorDomain same_identity{parameters.id, parameters.alternate_name
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

      /* Act */
      const auto equal = domain == same_identity;
      const auto unequal = domain != same_identity;

      /* Expect */
      EXPECT_TRUE(equal);
      EXPECT_FALSE(unequal);
    }

    std::string ErrorDomainIdentityCaseName_SameIdentifiersCompareEqual(
      const ::testing::TestParamInfo<ErrorDomainIdentityCase_SameIdentifiersCompareEqual>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      StableDomainIdentities, AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_SameIdentifiersCompareEqual,
      ::testing::Values(
        ErrorDomainIdentityCase_SameIdentifiersCompareEqual{0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain",
                                                            0x0000000000000002ULL, "CoreDomainStableIdentity",
                                                            "Core domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_SameIdentifiersCompareEqual{
          0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain", 0x0000000000000101ULL,
          "CommunicationDomainStableIdentity", "Communication domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_SameIdentifiersCompareEqual{
          0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain", 0x8000000000000002ULL,
          "ExecutionDomainStableHighUnsignedIdentity",
          "Execution domain has a stable high unsigned identity and non-empty name"}),
      ErrorDomainIdentityCaseName_SameIdentifiersCompareEqual);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that different domain identifiers establish inequality.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_DifferentIdentifiersCompareUnequal
        : public ::testing::TestWithParam<ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal>
    {
    };

    /* Verify that different domain identifiers establish inequality.
     * 1. Arrange: Read the case inputs and construct the domains needed for this check.
     * 2. Act: Evaluate equality and inequality for domains with different identifiers.
     * 3. Expect: Equality is false and inequality is true.
     */
    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_DifferentIdentifiersCompareUnequal,
           DifferentIdentifiersCompareUnequal)
    {
      /* Arrange */
      const ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal& parameters{GetParam()};
      const ErrorDomain domain{parameters.id, parameters.name
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
      const ErrorDomain different_identity{parameters.different_id, parameters.name
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

      /* Act */
      const auto equal = domain == different_identity;
      const auto unequal = domain != different_identity;

      /* Expect */
      EXPECT_FALSE(equal);
      EXPECT_TRUE(unequal);
    }

    std::string ErrorDomainIdentityCaseName_DifferentIdentifiersCompareUnequal(
      const ::testing::TestParamInfo<ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(StableDomainIdentities,
                             AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_DifferentIdentifiersCompareUnequal,
                             ::testing::Values(
                               ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal{
                                 0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain", 0x0000000000000002ULL,
                                 "CoreDomainStableIdentity", "Core domain has a stable identity and non-empty name"},
                               ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal{
                                 0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain",
                                 0x0000000000000101ULL, "CommunicationDomainStableIdentity",
                                 "Communication domain has a stable identity and non-empty name"},
                               ErrorDomainIdentityCase_DifferentIdentifiersCompareUnequal{
                                 0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain",
                                 0x8000000000000002ULL, "ExecutionDomainStableHighUnsignedIdentity",
                                 "Execution domain has a stable high unsigned identity and non-empty name"}),
                             ErrorDomainIdentityCaseName_DifferentIdentifiersCompareUnequal);

    /* ----------------------------------------------------------------------------------- */

    struct ErrorDomainIdentityCase_EmptyNameTerminates
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorDomainIdentityCase_EmptyNameTerminates& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that an empty domain name terminates the process.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_EmptyNameTerminates
        : public ::testing::TestWithParam<ErrorDomainIdentityCase_EmptyNameTerminates>
    {
    };

    /* Verify that an empty domain name terminates the process.
     * 1. Arrange: Read the domain identifier for this case.
     * 2. Act: Define construction with an empty name for execution in the death-test child.
     * 3. Expect: Executing that construction terminates the child process.
     */
    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_EmptyNameTerminates, EmptyNameTerminates)
    {
      /* Arrange */
      const ErrorDomainIdentityCase_EmptyNameTerminates& parameter{GetParam()};

      /* Act */
      const auto construct_invalid_domain = [&parameter]()
      {
        const ErrorDomain invalid_domain{parameter.id, std::string_view{}
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
                                         ,
                                         [](const ErrorCode& error)
                                         {
                                           struct EmptyNameDomainException
                                           {
                                               ErrorCode code;
                                           };
                                           throw EmptyNameDomainException{error};
                                         }
#endif
        };
        static_cast<void>(invalid_domain);
      };

      /* Expect */
      EXPECT_DEATH(construct_invalid_domain(), "");
    }

    std::string ErrorDomainIdentityCaseName_EmptyNameTerminates(
      const ::testing::TestParamInfo<ErrorDomainIdentityCase_EmptyNameTerminates>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      StableDomainIdentities, AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName_EmptyNameTerminates,
      ::testing::Values(
        ErrorDomainIdentityCase_EmptyNameTerminates{0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain",
                                                    0x0000000000000002ULL, "CoreDomainStableIdentity",
                                                    "Core domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_EmptyNameTerminates{
          0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain", 0x0000000000000101ULL,
          "CommunicationDomainStableIdentity", "Communication domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase_EmptyNameTerminates{
          0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain", 0x8000000000000002ULL,
          "ExecutionDomainStableHighUnsignedIdentity",
          "Execution domain has a stable high unsigned identity and non-empty name"}),
      ErrorDomainIdentityCaseName_EmptyNameTerminates);

    /* ----------------------------------------------------------------------------------- */

    /* Verify the published AdaptivePi domain identity remains stable and self-equal.
     * 1. Arrange: Capture the singleton domain and expected metadata.
     * 2. Act: Read its identifier and name and compare it to itself.
     * 3. Expect: The project-owned identity matches the published contract and is self-equal.
     */
    TEST(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, AdaptivePiDomainIdentityIsStable)
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

    /* ======================== End Test_AP_R3_CORE_002 ================================== */

    /* ========================== Test_AP_R3_CORE_008 ==================================== */

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    /* ----------------------------------------------------------------------------------- */

    /* Verify the published AdaptivePi exception preserves both its original error and message.
     * 1. Arrange: Construct the error and wrap it in the project-owned exception type.
     * 2. Act: Retrieve the stored error and textual message from the exception.
     * 3. Expect: The payload and message match the project contract exactly.
     */
    TEST(AP_R3_CORE_008_AdaptivePiException, AdaptivePiExceptionPreservesMetadata)
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
    TEST(AP_R3_CORE_008_AdaptivePiException, AdaptivePiExceptionMessageMatchesContract)
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

    /* ======================== End Test_AP_R3_CORE_008 ================================== */

  } /* namespace */

} /* namespace ara::core */
