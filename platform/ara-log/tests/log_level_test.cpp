#include "ara/log/log_fwd.h"
#include "ara/log/log_level.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <type_traits>

namespace
{
  using ara::log::LogLevel;

  /* ================================ Test_AP_R3_LOG_001 =============================== */

  static_assert(std::is_same_v<std::underlying_type_t<LogLevel>, std::uint8_t>);
  static_assert(!std::is_convertible_v<LogLevel, std::uint8_t>);
  static_assert(static_cast<std::uint8_t>(LogLevel::kOff) == 0x00);
  static_assert(static_cast<std::uint8_t>(LogLevel::kFatal) == 0x01);
  static_assert(static_cast<std::uint8_t>(LogLevel::kError) == 0x02);
  static_assert(static_cast<std::uint8_t>(LogLevel::kWarn) == 0x03);
  static_assert(static_cast<std::uint8_t>(LogLevel::kInfo) == 0x04);
  static_assert(static_cast<std::uint8_t>(LogLevel::kDebug) == 0x05);
  static_assert(static_cast<std::uint8_t>(LogLevel::kVerbose) == 0x06);

  struct LevelCase
  {
      const char* name;
      LogLevel level;
      std::uint8_t expected;
  };

  void operator<<(std::ostream& out, const LevelCase& value)
  {
    out << value.name << " expected=" << static_cast<unsigned>(value.expected);
  }

  /* Each case supplies one required severity and its independently specified value. */
  class AP_R3_LOG_001_LogLevelsHaveExpectedValues : public testing::TestWithParam<LevelCase>
  {
  };

  /* ----------------------------------------------------------------------------------- */

  /* Verify the numeric encoding of every approved severity.
   * 1. Arrange: Obtain a named severity and its required numeric value.
   * 2. Act: Convert the severity to its underlying byte representation.
   * 3. Expect: The byte equals the requirement's value.
   */
  TEST_P(AP_R3_LOG_001_LogLevelsHaveExpectedValues, HasRequiredEncoding)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    /* Act */
    const auto value = static_cast<std::uint8_t>(parameter.level);
    /* Expect */
    EXPECT_EQ(value, parameter.expected);
  }

  std::string LevelCaseName(const testing::TestParamInfo<LevelCase>& info)
  {
    return info.param.name;
  }

  INSTANTIATE_TEST_SUITE_P(
    RequiredLevels, AP_R3_LOG_001_LogLevelsHaveExpectedValues,
    testing::Values(LevelCase{"Off", LogLevel::kOff, 0x00}, LevelCase{"Fatal", LogLevel::kFatal, 0x01},
                    LevelCase{"Error", LogLevel::kError, 0x02}, LevelCase{"Warn", LogLevel::kWarn, 0x03},
                    LevelCase{"Info", LogLevel::kInfo, 0x04}, LevelCase{"Debug", LogLevel::kDebug, 0x05},
                    LevelCase{"Verbose", LogLevel::kVerbose, 0x06}),
    LevelCaseName);

  /* ============================== End Test_AP_R3_LOG_001 ============================= */

} // namespace
