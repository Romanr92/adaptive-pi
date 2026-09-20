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
    struct DomainCase
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const DomainCase& parameter)
    {
      return output << parameter.description;
    }

    class AP_R3_CORE_001_ErrorDomainProvidesErrorContext : public ::testing::TestWithParam<DomainCase>
    {
    };

    TEST_P(AP_R3_CORE_001_ErrorDomainProvidesErrorContext, ReturnsConfiguredIdentifier)
    {
      const DomainCase& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name};

      EXPECT_EQ(domain.Id(), parameter.id);
    }

    TEST_P(AP_R3_CORE_001_ErrorDomainProvidesErrorContext, ReturnsConfiguredName)
    {
      const DomainCase& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name};

      EXPECT_EQ(domain.Name(), parameter.name);
    }

    std::string DomainCaseName(const ::testing::TestParamInfo<DomainCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ProjectDomainContexts, AP_R3_CORE_001_ErrorDomainProvidesErrorContext,
      ::testing::Values(
        DomainCase{0x0000000000000001ULL, "CoreDomain", "CoreDomainErrorContext", "Core domain error context"},
        DomainCase{0x0000000000000100ULL, "CommunicationDomain", "CommunicationDomainErrorContext",
                   "Communication domain error context"},
        DomainCase{0x8000000000000001ULL, "ExecutionDomain", "ExecutionDomainHighUnsignedIdentifierContext",
                   "Execution domain error context with a high unsigned identifier"}),
      DomainCaseName);
    /* ======================== End Test_AP_R3_CORE_001 ================================== */

    /* ========================== Test_AP_R3_CORE_002 ==================================== */
    struct ErrorDomainIdentityCase
    {
        ErrorDomain::IdType id;
        std::string_view name;
        std::string_view alternate_name;
        ErrorDomain::IdType different_id;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ErrorDomainIdentityCase& parameter)
    {
      return output << parameter.description;
    }

    constexpr ErrorDomain compile_time_domain{0x0000000000000200ULL, "CompileTimeDomain"};

    constexpr ErrorDomain same_compile_time_identity{0x0000000000000200ULL, "SameIdentityDifferentName"};

    constexpr ErrorDomain different_compile_time_identity{0x0000000000000201ULL, "DifferentIdentity"};

    static_assert(compile_time_domain.Id() == 0x0000000000000200ULL,
                  "ErrorDomain identifier access must work in a constant expression");

    static_assert(!compile_time_domain.Name().empty(), "A compile-time ErrorDomain must have a non-empty name");

    static_assert(compile_time_domain == same_compile_time_identity,
                  "Domains with the same identifier must compare equal at compile time");

    static_assert(compile_time_domain != different_compile_time_identity,
                  "Domains with different identifiers must compare unequal at compile time");

    class AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName : public ::testing::TestWithParam<ErrorDomainIdentityCase>
    {
    };

    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, PreservesConfiguredIdentifier)
    {
      const ErrorDomainIdentityCase& parameter{GetParam()};
      const ErrorDomain first_domain{parameter.id, parameter.name};
      const ErrorDomain reconstructed_domain{parameter.id, parameter.name};

      EXPECT_EQ(first_domain.Id(), parameter.id);
      EXPECT_EQ(reconstructed_domain.Id(), first_domain.Id());
    }

    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, ProvidesNonEmptyName)
    {
      const ErrorDomainIdentityCase& parameter{GetParam()};
      const ErrorDomain domain{parameter.id, parameter.name};

      EXPECT_FALSE(domain.Name().empty());
    }

    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, SameIdentifiersCompareEqual)
    {
      const ErrorDomainIdentityCase& parameters{GetParam()};
      const ErrorDomain domain{parameters.id, parameters.name};
      const ErrorDomain same_identity{parameters.id, parameters.alternate_name};

      EXPECT_TRUE(domain == same_identity);
      EXPECT_FALSE(domain != same_identity);
    }

    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, DifferentIdentifiersCompareUnequal)
    {
      const ErrorDomainIdentityCase& parameters{GetParam()};
      const ErrorDomain domain{parameters.id, parameters.name};
      const ErrorDomain different_identity{parameters.different_id, parameters.name};

      EXPECT_FALSE(domain == different_identity);
      EXPECT_TRUE(domain != different_identity);
    }

    TEST_P(AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName, EmptyNameTerminates)
    {
      const ErrorDomainIdentityCase& parameter{GetParam()};

      EXPECT_DEATH(
        {
          const ErrorDomain invalid_domain(parameter.id, std::string_view{});
          static_cast<void>(invalid_domain);
        },
        "");
    }

    std::string ErrorDomainIdentityCaseName(const ::testing::TestParamInfo<ErrorDomainIdentityCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      StableDomainIdentities, AP_R3_CORE_002_ErrorDomainHasStableIdentityAndName,
      ::testing::Values(
        ErrorDomainIdentityCase{0x0000000000000001ULL, "CoreDomain", "RenamedCoreDomain", 0x0000000000000002ULL,
                                "CoreDomainStableIdentity", "Core domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase{0x0000000000000100ULL, "CommunicationDomain", "RenamedCommunicationDomain",
                                0x0000000000000101ULL, "CommunicationDomainStableIdentity",
                                "Communication domain has a stable identity and non-empty name"},
        ErrorDomainIdentityCase{0x8000000000000001ULL, "ExecutionDomain", "RenamedExecutionDomain",
                                0x8000000000000002ULL, "ExecutionDomainStableHighUnsignedIdentity",
                                "Execution domain has a stable high unsigned identity and non-empty name"}),
      ErrorDomainIdentityCaseName);
    /* ======================== End Test_AP_R3_CORE_002 ================================== */

  } // namespace

} // namespace ara::core
