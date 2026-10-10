#include "ara/log/logger.h"

#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>

namespace
{
  using ara::log::LogLevel;

  /* ================================ Test_AP_R3_LOG_002 =============================== */

  struct ContextCase
  {
      const char* name;        // Case description
      const char* id;          // Input context ID
      const char* description; // Input context description
      LogLevel threshold;      // Input log threshold
  };

  void operator<<(std::ostream& out, const ContextCase& value)
  {
    out << value.name << " id=" << value.id << " description=" << value.description;
  }

  /* Cases supply unique IDs and all seven thresholds; GetParam provides each context. */
  class AP_R3_LOG_002_LoggerRetainsContextProperties : public testing::TestWithParam<ContextCase>
  {
  };

  /* ----------------------------------------------------------------------------------- */

  /* Verify that registration and reference lookup retain all independent inputs.
   * 1. Arrange: Obtain a unique context and register it through the recoverable factory.
   * 2. Act: Retrieve that context using the reference-returning factory.
   * 3. Expect: Creation succeeds and all three properties equal their supplied values.
   */
  TEST_P(AP_R3_LOG_002_LoggerRetainsContextProperties, RetainsAllInputs)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = ara::log::TryCreateLogger(parameter.id, parameter.description, parameter.threshold);
    /* Act */
    auto* logger =
      result.HasValue() ? &ara::log::CreateLogger(parameter.id, parameter.description, parameter.threshold) : nullptr;
    /* Expect */
    ASSERT_TRUE(result.HasValue());
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->ContextId(), parameter.id);
    EXPECT_EQ(logger->ContextDescription(), parameter.description);
    EXPECT_EQ(logger->DefaultThreshold(), parameter.threshold);
  }

  std::string ContextCaseName(const testing::TestParamInfo<ContextCase>& info)
  {
    return info.param.name;
  }

  INSTANTIATE_TEST_SUITE_P(ContextInputs, AP_R3_LOG_002_LoggerRetainsContextProperties,
                           testing::Values(ContextCase{"Off", "T200", "", LogLevel::kOff},
                                           ContextCase{"Fatal", "T201", "Context Fatal", LogLevel::kFatal},
                                           ContextCase{"Error", "T202", "Context Error", LogLevel::kError},
                                           ContextCase{"Warn", "T203", "Context Warn", LogLevel::kWarn},
                                           ContextCase{"Info", "T204", "Context Info", LogLevel::kInfo},
                                           ContextCase{"Debug", "T205", "Context Debug", LogLevel::kDebug},
                                           ContextCase{"Verbose", "T206", "Context Verbose", LogLevel::kVerbose}),
                           ContextCaseName);

  /* ----------------------------------------------------------------------------------- */

  /* Verify ownership rather than merely retaining a view into caller memory.
   * 1. Arrange: Supply bounded views into mutable strings with unused suffixes.
   * 2. Act: Create the logger, change both strings, then destroy their storage.
   * 3. Expect: The logger retains exactly the original view contents and threshold.
   */
  TEST(AP_R3_LOG_002_ContextOwnsInputStrings, SurvivesInputMutationAndDestruction)
  {
    /* Arrange */
    ara::log::Logger* logger = nullptr;
    bool created = false;
    /* Act */
    {
      std::string id = "OWN2_unused";
      std::string description = "Owned description_unused";
      auto result = ara::log::TryCreateLogger(std::string_view{id.data(), 4}, std::string_view{description.data(), 17},
                                              LogLevel::kInfo);
      created = result.HasValue();
      if (created)
      {
        logger = &result.Value().get();
      }
      id.assign(id.size(), 'X');
      description.assign(description.size(), 'Y');
    }
    /* Expect */
    ASSERT_TRUE(created);
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->ContextId(), "OWN2");
    EXPECT_EQ(logger->ContextDescription(), "Owned description");
    EXPECT_EQ(logger->DefaultThreshold(), LogLevel::kInfo);
  }

  /* ============================== End Test_AP_R3_LOG_002 ============================= */

} // namespace
