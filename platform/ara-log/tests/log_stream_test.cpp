/* Include only the public logger header to catch incomplete stream API regressions. */
#include "ara/log/logger.h"

#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
  /* =============================== Test_AP_R3_LOG_005 ================================ */

  using ara::log::Logger;
  using ara::log::LogStream;
  /* LOG-005 stream insertion must compile with only logger.h included. */
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogInfo() << "message"), LogStream&>);
  static_assert(!std::is_default_constructible_v<LogStream>);
  static_assert(!std::is_copy_constructible_v<LogStream>);
  static_assert(!std::is_copy_assignable_v<LogStream>);
  static_assert(std::is_nothrow_move_constructible_v<LogStream>);
  static_assert(!std::is_move_assignable_v<LogStream>);
  static_assert(std::is_nothrow_destructible_v<LogStream>);
  static_assert(noexcept(std::declval<LogStream&>().Flush()));
  static_assert(std::is_same_v<decltype(std::declval<LogStream&>().Flush()), void>);

  template <typename T, typename = void>
  struct SupportsInsertion : std::false_type
  {
  };
  template <typename T>
  struct SupportsInsertion<T, std::void_t<decltype(std::declval<LogStream&>() << std::declval<T>())>>
      : std::bool_constant<std::is_same_v<decltype(std::declval<LogStream&>() << std::declval<T>()), LogStream&> &&
                           noexcept(std::declval<LogStream&>() << std::declval<T>())>
  {
  };

  static_assert(SupportsInsertion<bool>::value);
  static_assert(SupportsInsertion<char>::value);
  static_assert(SupportsInsertion<signed char>::value);
  static_assert(SupportsInsertion<unsigned char>::value);
  static_assert(SupportsInsertion<short>::value);
  static_assert(SupportsInsertion<unsigned short>::value);
  static_assert(SupportsInsertion<int>::value);
  static_assert(SupportsInsertion<unsigned int>::value);
  static_assert(SupportsInsertion<long>::value);
  static_assert(SupportsInsertion<unsigned long>::value);
  static_assert(SupportsInsertion<long long>::value);
  static_assert(SupportsInsertion<unsigned long long>::value);
  static_assert(SupportsInsertion<float>::value);
  static_assert(SupportsInsertion<double>::value);
  static_assert(SupportsInsertion<const char (&)[6]>::value);
  static_assert(SupportsInsertion<char*>::value);
  static_assert(SupportsInsertion<const char*>::value);
  static_assert(SupportsInsertion<std::string>::value);
  static_assert(SupportsInsertion<const std::string&>::value);
  static_assert(SupportsInsertion<std::string_view>::value);

  struct UnsupportedValue
  {
  };
  enum class UnsupportedEnum : std::uint8_t
  {
    kValue
  };
  static_assert(!SupportsInsertion<UnsupportedValue>::value);
  static_assert(!SupportsInsertion<UnsupportedEnum>::value);
  static_assert(!SupportsInsertion<wchar_t>::value);
  static_assert(!SupportsInsertion<char16_t>::value);
  static_assert(!SupportsInsertion<char32_t>::value);
  static_assert(!SupportsInsertion<const wchar_t*>::value);
  static_assert(!SupportsInsertion<const char16_t*>::value);
  static_assert(!SupportsInsertion<const char32_t*>::value);
  static_assert(!SupportsInsertion<long double>::value);
  static_assert(!SupportsInsertion<void*>::value);
  static_assert(!SupportsInsertion<std::nullptr_t>::value);

  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogFatal()), LogStream>);
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogError()), LogStream>);
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogWarn()), LogStream>);
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogInfo()), LogStream>);
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogDebug()), LogStream>);
  static_assert(std::is_same_v<decltype(std::declval<const Logger&>().LogVerbose()), LogStream>);
  static_assert(
    std::is_same_v<decltype(std::declval<const Logger&>().WithLevel(ara::log::LogLevel::kInfo)), LogStream>);

  struct EmptyStreamCase
  {
      const char* name;          // Case description
      void (*exercise)(Logger&); // Input stream lifecycle operation
  };
  void operator<<(std::ostream& out, const EmptyStreamCase& value)
  {
    out << value.name;
  }
  /* GetParam() supplies an empty-stream lifecycle; no console rendering is required. */
  class AP_R3_LOG_005_EmptyStreams : public testing::TestWithParam<EmptyStreamCase>
  {
  };

  /* ----------------------------------------------------------------------------------- */

  /* Verify that empty streams produce no console output.
   * 1. Arrange: Register a unique logger and select an empty-stream scenario.
   * 2. Act: Exercise insertion, flushing, or moving without adding message bytes.
   * 3. Expect: Stdout remains empty after all stream destruction completes.
   */
  TEST_P(AP_R3_LOG_005_EmptyStreams, EmitsNoOutput)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"EMPTY_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Empty stream test", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    testing::internal::CaptureStdout();
    parameter.exercise(result.Value().get());
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_TRUE(output.empty());
  }
  std::string EmptyStreamCaseName(const testing::TestParamInfo<EmptyStreamCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(EmptyLifetimes, AP_R3_LOG_005_EmptyStreams,
                           testing::Values(EmptyStreamCase{"Destruction",
                                                           [](Logger& logger)
                                                           {
                                                             auto stream = logger.LogInfo();
                                                             (void)stream;
                                                           }},
                                           EmptyStreamCase{"RepeatedFlush",
                                                           [](Logger& logger)
                                                           {
                                                             auto stream = logger.LogInfo();
                                                             stream.Flush();
                                                             stream.Flush();
                                                           }},
                                           EmptyStreamCase{"EmptyValues",
                                                           [](Logger& logger)
                                                           {
                                                             auto stream = logger.LogInfo();
                                                             stream << "" << std::string{} << std::string_view{};
                                                             stream.Flush();
                                                           }},
                                           EmptyStreamCase{"Move",
                                                           [](Logger& logger)
                                                           {
                                                             auto source = logger.LogInfo();
                                                             auto destination = std::move(source);
                                                             destination.Flush();
                                                           }}),
                           EmptyStreamCaseName);

  /* ============================= End Test_AP_R3_LOG_005 ============================== */
} // namespace
