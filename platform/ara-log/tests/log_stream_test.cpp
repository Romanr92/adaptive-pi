#include "ara/log/log_stream.h"
#include "ara/log/logger.h"

#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
  /* =============================== Test_AP_R3_LOG_005 ================================ */

  using ara::log::LogStream;
  static_assert(!std::is_copy_constructible_v<LogStream>);
  static_assert(!std::is_copy_assignable_v<LogStream>);
  static_assert(std::is_nothrow_move_constructible_v<LogStream>);
  static_assert(!std::is_move_assignable_v<LogStream>);

  template <typename T, typename = void>
  struct SupportsInsertion : std::false_type
  {
  };
  template <typename T>
  struct SupportsInsertion<T, std::void_t<decltype(std::declval<LogStream&>() << std::declval<T>())>> : std::true_type
  {
  };
  static_assert(SupportsInsertion<const char (&)[6]>::value);
  static_assert(SupportsInsertion<char*>::value);
  static_assert(!SupportsInsertion<wchar_t>::value);
  static_assert(!SupportsInsertion<char16_t>::value);
  static_assert(!SupportsInsertion<char32_t>::value);
  static_assert(!SupportsInsertion<long double>::value);
  static_assert(!SupportsInsertion<void*>::value);
  static_assert(!SupportsInsertion<std::nullptr_t>::value);

  struct InsertionCase
  {
      const char* name;
      void (*insert)(LogStream&);
      const char* expected;
  };
  void operator<<(std::ostream& out, const InsertionCase& value)
  {
    out << value.name << ": " << value.expected;
  }
  /* GetParam() supplies the insertion operation and independently specified message bytes. */
  class AP_R3_LOG_005_StreamInsertionBuildsRecord : public testing::TestWithParam<InsertionCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */

  /* Verify supported values through their emitted message rather than private buffer state.
   * 1. Arrange: Register a unique logger and read the insertion case.
   * 2. Act: Insert the value and destroy the stream while capturing stdout.
   * 3. Expect: Exactly one record ends with the specified message bytes.
   */
  TEST_P(AP_R3_LOG_005_StreamInsertionBuildsRecord, FormatsSupportedValues)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"INSERT_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Insertion test", ara::log::LogLevel::kVerbose);
    ara::log::Logger& logger = result.Value().get();
    /* Act */
    testing::internal::CaptureStdout();
    {
      auto stream = logger.LogInfo();
      parameter.insert(stream);
    }
    const int flush_status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    const std::string suffix = "] " + std::string{parameter.expected} + "\n";
    EXPECT_EQ(flush_status, 0);
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  std::string InsertionCaseName(const testing::TestParamInfo<InsertionCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(SupportedValues, AP_R3_LOG_005_StreamInsertionBuildsRecord,
                           testing::Values(InsertionCase{"ChainedValues",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << "ready=" << true << ',' << -42 << ',' << 1.5;
                                                         },
                                                         "ready=true,-42,1.5"},
                                           InsertionCase{"BoolTrue",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << true;
                                                         },
                                                         "true"},
                                           InsertionCase{"BoolFalse",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << false;
                                                         },
                                                         "false"},
                                           InsertionCase{"Character",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << 'Q';
                                                         },
                                                         "Q"},
                                           InsertionCase{"CString",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << "hello";
                                                         },
                                                         "hello"},
                                           InsertionCase{"String",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::string{"hello"};
                                                         },
                                                         "hello"},
                                           InsertionCase{"StringView",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::string_view{"helloTAIL", 5};
                                                         },
                                                         "hello"},
                                           InsertionCase{"Float",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << 1.23456789F;
                                                         },
                                                         "1.23456788"},
                                           InsertionCase{"Double",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << 1.2345678901234567;
                                                         },
                                                         "1.2345678901234567"},
                                           InsertionCase{"FloatNegativeZero",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << -0.0F;
                                                         },
                                                         "-0"},
                                           InsertionCase{"DoubleNegativeZero",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << -0.0;
                                                         },
                                                         "-0"},
                                           InsertionCase{"FloatNaN",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<float>::quiet_NaN();
                                                         },
                                                         "nan"},
                                           InsertionCase{"FloatInfinity",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<float>::infinity();
                                                         },
                                                         "inf"},
                                           InsertionCase{"FloatNegativeInfinity",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << -std::numeric_limits<float>::infinity();
                                                         },
                                                         "-inf"},
                                           InsertionCase{"DoubleNaN",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<double>::quiet_NaN();
                                                         },
                                                         "nan"},
                                           InsertionCase{"DoubleInfinity",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<double>::infinity();
                                                         },
                                                         "inf"},
                                           InsertionCase{"DoubleNegativeInfinity",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << -std::numeric_limits<double>::infinity();
                                                         },
                                                         "-inf"},
                                           InsertionCase{"SignedCharLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<signed char>::lowest();
                                                         },
                                                         "-128"},
                                           InsertionCase{"SignedCharMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<signed char>::max();
                                                         },
                                                         "127"},
                                           InsertionCase{"UnsignedCharLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned char>::lowest();
                                                         },
                                                         "0"},
                                           InsertionCase{"UnsignedCharMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned char>::max();
                                                         },
                                                         "255"},
                                           InsertionCase{"ShortLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<short>::lowest();
                                                         },
                                                         "-32768"},
                                           InsertionCase{"ShortMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<short>::max();
                                                         },
                                                         "32767"},
                                           InsertionCase{"UnsignedShortLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned short>::lowest();
                                                         },
                                                         "0"},
                                           InsertionCase{"UnsignedShortMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned short>::max();
                                                         },
                                                         "65535"},
                                           InsertionCase{"IntLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<int>::lowest();
                                                         },
                                                         "-2147483648"},
                                           InsertionCase{"IntMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<int>::max();
                                                         },
                                                         "2147483647"},
                                           InsertionCase{"UnsignedIntLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned int>::lowest();
                                                         },
                                                         "0"},
                                           InsertionCase{"UnsignedIntMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned int>::max();
                                                         },
                                                         "4294967295"},
                                           InsertionCase{"LongLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<long>::lowest();
                                                         },
                                                         "-9223372036854775808"},
                                           InsertionCase{"LongMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<long>::max();
                                                         },
                                                         "9223372036854775807"},
                                           InsertionCase{"UnsignedLongLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned long>::lowest();
                                                         },
                                                         "0"},
                                           InsertionCase{"UnsignedLongMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned long>::max();
                                                         },
                                                         "18446744073709551615"},
                                           InsertionCase{"LongLongLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<long long>::lowest();
                                                         },
                                                         "-9223372036854775808"},
                                           InsertionCase{"LongLongMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<long long>::max();
                                                         },
                                                         "9223372036854775807"},
                                           InsertionCase{"UnsignedLongLongLowest",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned long long>::lowest();
                                                         },
                                                         "0"},
                                           InsertionCase{"UnsignedLongLongMax",
                                                         [](LogStream& stream)
                                                         {
                                                           stream << std::numeric_limits<unsigned long long>::max();
                                                         },
                                                         "18446744073709551615"}),
                           InsertionCaseName);
  /* ----------------------------------------------------------------------------------- */

  /* Verify that move construction transfers pending content and submission ownership.
   * 1. Arrange: Register a unique logger for this lifetime-order scenario.
   * 2. Act: Move a stream containing bytes, append more bytes, and destroy both objects.
   * 3. Expect: Exactly one record contains the original and appended content in order.
   */
  TEST(AP_R3_LOG_005_MoveTransfersPendingRecord, DestinationDestructionSubmitsOnce)
  {
    /* Arrange */
    auto result =
      ara::log::TryCreateLogger("MOVE_DestinationDestructionSubmitsOnce", "Move test", ara::log::LogLevel::kInfo);
    ara::log::Logger& logger = result.Value().get();
    /* Act */
    testing::internal::CaptureStdout();
    {
      auto source = logger.LogInfo();
      source << "pending";
      {
        auto destination = std::move(source);
        destination << " tail";
      }
    }
    const int flush_status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    const std::string suffix = "] pending tail\n";
    EXPECT_EQ(flush_status, 0);
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  /* ----------------------------------------------------------------------------------- */

  /* Verify that move construction transfers pending content and submission ownership.
   * 1. Arrange: Register a unique logger for this lifetime-order scenario.
   * 2. Act: Move a stream containing bytes, append more bytes, and destroy both objects.
   * 3. Expect: Exactly one record contains the original and appended content in order.
   */
  TEST(AP_R3_LOG_005_MoveTransfersPendingRecord, SourceDestructionLeavesDestinationPending)
  {
    /* Arrange */
    auto result = ara::log::TryCreateLogger("MOVE_SourceDestructionLeavesDestinationPending", "Move test",
                                            ara::log::LogLevel::kInfo);
    ara::log::Logger& logger = result.Value().get();
    /* Act */
    testing::internal::CaptureStdout();
    {
      auto destination = [&logger]()
      {
        auto source = logger.LogInfo();
        source << "pending";
        return LogStream{std::move(source)};
      }();
      destination << " tail";
    }
    const int flush_status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    const std::string suffix = "] pending tail\n";
    EXPECT_EQ(flush_status, 0);
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  /* ============================= End Test_AP_R3_LOG_005 ============================== */
} // namespace
