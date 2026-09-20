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

  } // namespace

} // namespace ara::core
