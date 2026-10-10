#include "console_record_formatter.h"
#include "linux_metadata_provider.h"

#include <cstdint>
#include <ctime>
#include <gtest/gtest.h>
#include <ostream>
#include <string>

namespace
{
  timespec supplied_time{};
}

/* The wrapper is confined to this executable and supplies deterministic realtime metadata. */
/* GNU ld requires these reserved identifiers for --wrap. */
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" int __real_clock_gettime(clockid_t clock, timespec* value);
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" int __wrap_clock_gettime(clockid_t clock, timespec* value)
{
  if (clock == CLOCK_REALTIME)
  {
    *value = supplied_time;
    return 0;
  }
  return __real_clock_gettime(clock, value);
}

namespace
{
  /* =============================== Test_AP_R3_LOG_006 ================================ */
  struct TimestampTruncationCase
  {
      const char* name;          // Case description
      long nanoseconds;          // Input nanoseconds within supplied second
      std::int64_t milliseconds; // Expected milliseconds since Unix epoch
      const char* timestamp;     // Expected UTC timestamp
  };
  void operator<<(std::ostream& out, const TimestampTruncationCase& value)
  {
    out << value.name;
  }
  /* GetParam supplies submillisecond and rollover boundaries for the production provider. */
  class AP_R3_LOG_006_TruncatesTimestamp : public testing::TestWithParam<TimestampTruncationCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify production metadata truncates nanoseconds instead of rounding them.
   * 1. Arrange: Supply a fixed second with the selected nanosecond fraction.
   * 2. Act: Read the Linux provider and render the resulting metadata.
   * 3. Expect: Milliseconds and the rendered UTC prefix match the truncated value.
   */
  TEST_P(AP_R3_LOG_006_TruncatesTimestamp, DoesNotRound)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    supplied_time = timespec{1, parameter.nanoseconds};
    ara::log::detail::LinuxMetadataProvider provider;
    ara::log::detail::RuntimeMetadata metadata{};
    ara::log::detail::RecordBuffer output;
    /* Act */
    const bool acquired = provider.Read(metadata);
    const bool rendered =
      acquired && ara::log::detail::RenderConsoleRecord(metadata, ara::log::LogLevel::kInfo, "TIME", "message", output);
    /* Expect */
    EXPECT_TRUE(acquired);
    EXPECT_TRUE(rendered);
    EXPECT_EQ(metadata.unix_milliseconds, parameter.milliseconds);
    const std::string expected_prefix = std::string{"["} + parameter.timestamp + "]";
    EXPECT_EQ(output.View().substr(0, expected_prefix.size()), expected_prefix);
  }
  std::string TimestampTruncationCaseName(const testing::TestParamInfo<TimestampTruncationCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    Fractions, AP_R3_LOG_006_TruncatesTimestamp,
    testing::Values(TimestampTruncationCase{"BelowMillisecond", 999999, 1000, "1970-01-01T00:00:01.000Z"},
                    TimestampTruncationCase{"ExactMillisecond", 1000000, 1001, "1970-01-01T00:00:01.001Z"},
                    TimestampTruncationCase{"AboveMillisecond", 1999999, 1001, "1970-01-01T00:00:01.001Z"},
                    TimestampTruncationCase{"BelowSecond", 999999999, 1999, "1970-01-01T00:00:01.999Z"}),
    TimestampTruncationCaseName);
  /* ============================= End Test_AP_R3_LOG_006 ============================== */
} // namespace
