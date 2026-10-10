#include "ara/log/logger.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <string>

namespace
{
  using ara::log::LogLevel;
  /* ================================ Test_AP_R3_LOG_002 =============================== */

  struct ThresholdCase
  {
      const char* name;      // Case description
      const char* id;        // Input context ID
      LogLevel threshold;    // Input log threshold
      unsigned enabled_mask; // Expected enabled-severity bitmask
  };
  void operator<<(std::ostream& out, const ThresholdCase& value)
  {
    out << value.name;
  }
  /* Parameterization checks each threshold against an independently enumerated enabled mask.
   * GetParam() supplies the current ThresholdCase, including the unique context ID,
   * configured threshold, and bit mask defining the expected enabled severities.
   */
  class AP_R3_LOG_002_ThresholdFiltersDefinedLevels : public testing::TestWithParam<ThresholdCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */

  /* Verify each threshold against every defined severity and every undefined byte value.
   * 1. Arrange: Register a unique context with the case threshold.
   * 2. Act: Query IsEnabled for all 256 underlying severity values.
   * 3. Expect: Exactly the independently supplied severity set is enabled.
   */
  TEST_P(AP_R3_LOG_002_ThresholdFiltersDefinedLevels, MatchesEnabledSeveritySet)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = ara::log::TryCreateLogger(parameter.id, "Threshold test", parameter.threshold);
    /* Act */
    std::array<bool, 256> enabled{};
    if (result.HasValue())
    {
      for (std::size_t i = 0; i < enabled.size(); ++i)
      {
        enabled[i] = result.Value().get().IsEnabled(static_cast<LogLevel>(i));
      }
    }
    /* Expect */
    ASSERT_TRUE(result.HasValue());
    for (std::size_t i = 0; i < enabled.size(); ++i)
    {
      const bool expected = i < 7 && (parameter.enabled_mask & (1U << i)) != 0;
      EXPECT_EQ(enabled[i], expected) << "severity=" << i;
    }
  }
  std::string ThresholdCaseName(const testing::TestParamInfo<ThresholdCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Thresholds, AP_R3_LOG_002_ThresholdFiltersDefinedLevels,
                           testing::Values(ThresholdCase{"Off", "F20", LogLevel::kOff, 0x00},
                                           ThresholdCase{"Fatal", "F21", LogLevel::kFatal, 0x02},
                                           ThresholdCase{"Error", "F22", LogLevel::kError, 0x06},
                                           ThresholdCase{"Warn", "F23", LogLevel::kWarn, 0x0e},
                                           ThresholdCase{"Info", "F24", LogLevel::kInfo, 0x1e},
                                           ThresholdCase{"Debug", "F25", LogLevel::kDebug, 0x3e},
                                           ThresholdCase{"Verbose", "F26", LogLevel::kVerbose, 0x7e}),
                           ThresholdCaseName);
  /* ----------------------------------------------------------------------------------- */
  /* Verify that undefined creation thresholds violate the context precondition.
   * 1. Arrange: Select the first undefined threshold and the largest byte value.
   * 2. Act: Prepare registration attempts with each invalid threshold.
   * 3. Expect: Both attempts terminate in isolated child processes.
   */
  TEST(AP_R3_LOG_002_InvalidCreationThreshold, Terminates)
  {
    /* Arrange */
    const auto first_invalid = static_cast<LogLevel>(0x07);
    const auto largest_invalid = static_cast<LogLevel>(0xff);
    /* Act */
    const auto first_attempt = [](LogLevel invalid)
    {
      (void)ara::log::TryCreateLogger("BADFIRST", "Invalid", invalid);
    };
    const auto largest_attempt = [](LogLevel invalid)
    {
      (void)ara::log::TryCreateLogger("BADLAST", "Invalid", invalid);
    };
    /* Expect */
    EXPECT_DEATH(first_attempt(first_invalid), "");
    EXPECT_DEATH(largest_attempt(largest_invalid), "");
  }

  /* ============================== End Test_AP_R3_LOG_002 ============================= */
} // namespace
