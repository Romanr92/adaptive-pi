#include "ara/log/log_stream.h"
#include "logger_registry.h"
#include "record_buffer.h"

#include <algorithm>
#include <array>
#include <clocale>
#include <condition_variable>
#include <cstdio>
#include <gtest/gtest.h>
#include <limits>
#include <locale>
#include <map>
#include <mutex>
#include <ostream>
#include <regex>
#include <string>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{
  using ara::log::Logger;
  using ara::log::LogLevel;
  using ara::log::LogStream;
  /* ==================================== Fixtures ===================================== */

  class FixedProvider final : public ara::log::detail::MetadataProvider
  {
    public:
      std::size_t reads = 0;
      bool Read(ara::log::detail::RuntimeMetadata& output) noexcept override
      {
        ++reads;
        output = {0, 12, 34};
        return true;
      }
  };
  class CollectingSink final : public ara::log::detail::Sink
  {
    public:
      std::size_t writes = 0;
      ara::log::detail::RecordBuffer record;
      ara::log::detail::SinkStatus Write(std::string_view bytes) noexcept override
      {
        ++writes;
        return record.Append(bytes) ? ara::log::detail::SinkStatus::kSuccess
                                    : ara::log::detail::SinkStatus::kWriteFailed;
      }
  };
  /* Fresh dependencies and registry isolate each case; registry destruction precedes borrowed dependencies. */
  class LoggingContractFixture : public testing::Test
  {
    protected:
      FixedProvider provider;
      CollectingSink sink;
      ara::log::detail::LoggerRegistry registry{sink, provider};
      Logger* logger = nullptr;
      void SetUp() override
      {
        auto result = registry.Register("VALUES", "Logging contract tests", LogLevel::kVerbose);
        ASSERT_TRUE(result.HasValue());
        logger = &result.Value().get();
      }
  };
  class CommaPunctuation final : public std::numpunct<char>
  {
    protected:
      char do_decimal_point() const override
      {
        return ',';
      }
      char do_thousands_sep() const override
      {
        return '.';
      }
      std::string do_grouping() const override
      {
        return "\3";
      }
  };
  /* Preserve the global C++ locale while exercising an installed-independent comma-decimal locale. */
  class LocaleFixture : public LoggingContractFixture
  {
    protected:
      std::locale previous;
      void SetUp() override
      {
        LoggingContractFixture::SetUp();
        previous = std::locale();
        std::locale::global(std::locale{previous, new CommaPunctuation});
      }
      void TearDown() override
      {
        std::locale::global(previous);
      }
  };
  /* Save LC_NUMERIC so locale selection and skipped tests restore process-global state. */
  class CNumericLocaleFixture : public LoggingContractFixture
  {
    protected:
      std::string previous_numeric;
      void SetUp() override
      {
        LoggingContractFixture::SetUp();
        previous_numeric = std::setlocale(LC_NUMERIC, nullptr);
      }
      void TearDown() override
      {
        EXPECT_NE(std::setlocale(LC_NUMERIC, previous_numeric.c_str()), nullptr);
      }
  };
  /* ================================== End Fixtures =================================== */
  /* =============================== Test_AP_R3_LOG_005 ================================ */

  struct ValueBoundaryCase
  {
      const char* name;           // Case description
      void (*insert)(LogStream&); // Input insertion operation
      std::string expected;       // Expected unescaped message
  };
  void operator<<(std::ostream& out, const ValueBoundaryCase& value)
  {
    out << value.name;
  }
  /* GetParam supplies a supported type's boundary values and independent expected text. */
  class AP_R3_LOG_005_SupportedInsertionTypes : public LoggingContractFixture,
                                                public testing::WithParamInterface<ValueBoundaryCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify every integral overload renders decimal limits and string inputs preserve byte lengths.
   * 1. Arrange: Select a type-specific insertion operation and expected payload.
   * 2. Act: Submit the values through an injected logger by destruction.
   * 3. Expect: Exactly one record contains the independently specified decimal or string bytes.
   */
  TEST_P(AP_R3_LOG_005_SupportedInsertionTypes, RendersBoundaryValues)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    /* Act */
    {
      auto stream = logger->LogInfo();
      parameter.insert(stream);
    }
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] " + parameter.expected + "\n");
  }
  std::string ValueBoundaryCaseName(const testing::TestParamInfo<ValueBoundaryCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(AllSupportedTypes, AP_R3_LOG_005_SupportedInsertionTypes, testing::Values(
ValueBoundaryCase{"SignedChar", [](LogStream& stream) { stream << std::numeric_limits<signed char>::min() << ' ' << std::numeric_limits<signed char>::max(); }, std::to_string(std::numeric_limits<signed char>::min()) + " " + std::to_string(std::numeric_limits<signed char>::max())},
ValueBoundaryCase{"UnsignedChar", [](LogStream& stream) { stream << std::numeric_limits<unsigned char>::min() << ' ' << std::numeric_limits<unsigned char>::max(); }, std::to_string(std::numeric_limits<unsigned char>::min()) + " " + std::to_string(std::numeric_limits<unsigned char>::max())},
ValueBoundaryCase{"Short", [](LogStream& stream) { stream << std::numeric_limits<short>::min() << ' ' << std::numeric_limits<short>::max(); }, std::to_string(std::numeric_limits<short>::min()) + " " + std::to_string(std::numeric_limits<short>::max())},
ValueBoundaryCase{"UnsignedShort", [](LogStream& stream) { stream << std::numeric_limits<unsigned short>::min() << ' ' << std::numeric_limits<unsigned short>::max(); }, std::to_string(std::numeric_limits<unsigned short>::min()) + " " + std::to_string(std::numeric_limits<unsigned short>::max())},
ValueBoundaryCase{"Int", [](LogStream& stream) { stream << std::numeric_limits<int>::min() << ' ' << std::numeric_limits<int>::max(); }, std::to_string(std::numeric_limits<int>::min()) + " " + std::to_string(std::numeric_limits<int>::max())},
ValueBoundaryCase{"UnsignedInt", [](LogStream& stream) { stream << std::numeric_limits<unsigned int>::min() << ' ' << std::numeric_limits<unsigned int>::max(); }, std::to_string(std::numeric_limits<unsigned int>::min()) + " " + std::to_string(std::numeric_limits<unsigned int>::max())},
ValueBoundaryCase{"Long", [](LogStream& stream) { stream << std::numeric_limits<long>::min() << ' ' << std::numeric_limits<long>::max(); }, std::to_string(std::numeric_limits<long>::min()) + " " + std::to_string(std::numeric_limits<long>::max())},
ValueBoundaryCase{"UnsignedLong", [](LogStream& stream) { stream << std::numeric_limits<unsigned long>::min() << ' ' << std::numeric_limits<unsigned long>::max(); }, std::to_string(std::numeric_limits<unsigned long>::min()) + " " + std::to_string(std::numeric_limits<unsigned long>::max())},
ValueBoundaryCase{"LongLong", [](LogStream& stream) { stream << std::numeric_limits<long long>::min() << ' ' << std::numeric_limits<long long>::max(); }, std::to_string(std::numeric_limits<long long>::min()) + " " + std::to_string(std::numeric_limits<long long>::max())},
ValueBoundaryCase{"UnsignedLongLong", [](LogStream& stream) { stream << std::numeric_limits<unsigned long long>::min() << ' ' << std::numeric_limits<unsigned long long>::max(); }, std::to_string(std::numeric_limits<unsigned long long>::min()) + " " + std::to_string(std::numeric_limits<unsigned long long>::max())},
ValueBoundaryCase{"MutableCString", [](LogStream& stream) { char text[] = "mutable"; stream << text; }, "mutable"},
ValueBoundaryCase{"StringNullByte", [](LogStream& stream) { stream << std::string{"a\0b", 3}; }, std::string{"a\0b", 3}},
ValueBoundaryCase{"ViewNullByte", [](LogStream& stream) { stream << std::string_view{"a\0b", 3}; }, std::string{"a\0b", 3}},
ValueBoundaryCase{"FloatNonFinite", [](LogStream& stream) { stream << std::numeric_limits<float>::infinity() << ' ' << -std::numeric_limits<float>::infinity() << ' ' << std::numeric_limits<double>::quiet_NaN(); }, "inf -inf nan"},
ValueBoundaryCase{"FloatNegativeZero", [](LogStream& stream) { stream << -0.0F; }, "-0"} ,
ValueBoundaryCase{"FloatMinimum", [](LogStream& stream) { stream << std::numeric_limits<float>::min(); }, "1.17549435e-38"},
ValueBoundaryCase{"FloatMaximum", [](LogStream& stream) { stream << std::numeric_limits<float>::max(); }, "3.40282347e+38"},
ValueBoundaryCase{"FloatSubnormal", [](LogStream& stream) { stream << std::numeric_limits<float>::denorm_min(); }, "1.40129846e-45"},
ValueBoundaryCase{"DoubleMinimum", [](LogStream& stream) { stream << std::numeric_limits<double>::min(); }, "2.2250738585072014e-308"},
ValueBoundaryCase{"DoubleMaximum", [](LogStream& stream) { stream << std::numeric_limits<double>::max(); }, "1.7976931348623157e+308"},
ValueBoundaryCase{"DoubleSubnormal", [](LogStream& stream) { stream << std::numeric_limits<double>::denorm_min(); }, "4.9406564584124654e-324"},
ValueBoundaryCase{"NegativeFloatMaximum", [](LogStream& stream) { stream << -std::numeric_limits<float>::max(); }, "-3.40282347e+38"},
ValueBoundaryCase{"NegativeDoubleMaximum", [](LogStream& stream) { stream << -std::numeric_limits<double>::max(); }, "-1.7976931348623157e+308"}
), ValueBoundaryCaseName);
  /* ----------------------------------------------------------------------------------- */
  /* Verify numeric insertion ignores locale grouping and decimal punctuation.
   * 1. Arrange: Use the fixture's comma decimal separator and grouped integer locale.
   * 2. Act: Submit an integer, float, and double through the stream.
   * 3. Expect: Decimal integer digits and dot decimal separators remain locale-independent.
   */
  TEST_F(LocaleFixture, AP_R3_LOG_005_LocaleIndependentFormatting)
  {
    /* Arrange */
    const char* expected = "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] 1234567 1.5 2.5\n";
    /* Act */
    logger->LogInfo() << 1234567 << ' ' << 1.5F << ' ' << 2.5;
    /* Expect */
    EXPECT_EQ(std::use_facet<std::numpunct<char>>(std::locale()).decimal_point(), ',');
    EXPECT_EQ(sink.record.View(), expected);
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify flush submits immediately, snapshots copied inputs, and does not replay on move or destruction.
   * 1. Arrange: Accumulate a string and mutate its source before submission.
   * 2. Act: Flush, inspect write count, move the empty stream, then destroy both streams.
   * 3. Expect: The original copied text is written once before Flush returns.
   */
  TEST_F(LoggingContractFixture, AP_R3_LOG_005_FlushConsumesCopiedMessage)
  {
    /* Arrange */
    std::string message{"original"};
    std::size_t writes_after_flush = 0;
    /* Act */
    {
      auto source = logger->LogInfo();
      source << message;
      message.assign("mutated");
      source.Flush();
      writes_after_flush = sink.writes;
      auto destination = std::move(source);
    }
    /* Expect */
    EXPECT_EQ(writes_after_flush, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] original\n");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify floating insertion ignores a real comma-decimal C numeric locale.
   * 1. Arrange: Select de_DE UTF-8 and confirm the C decimal separator is a comma.
   * 2. Act: Submit finite float and double values through the injected logger.
   * 3. Expect: The payload still uses dot decimal separators and max_digits10 precision.
   */
  TEST_F(CNumericLocaleFixture, AP_R3_LOG_005_CLocaleIndependentFormatting)
  {
    /* Arrange */
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") == nullptr && std::setlocale(LC_NUMERIC, "de_DE.utf8") == nullptr)
    {
      GTEST_SKIP() << "de_DE UTF-8 locale unavailable; both Host CI jobs provision it.";
    }
    ASSERT_STREQ(std::localeconv()->decimal_point, ",");
    /* Act */
    logger->LogInfo() << 0.1F << ' ' << 0.1;
    /* Expect */
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] 0.100000001 0.10000000000000001\n");
  }
  /* ============================= End Test_AP_R3_LOG_005 ============================== */
  /* =============================== Test_AP_R3_LOG_002 ================================ */

  struct FilteringMatrixCase
  {
      std::string name;   // Case description
      LogLevel threshold; // Input context threshold
      LogLevel severity;  // Input message severity
      bool admitted;      // Expected admission
      const char* label;  // Expected emitted severity name
  };
  void operator<<(std::ostream& out, const FilteringMatrixCase& value)
  {
    out << value.name;
  }
  /* GetParam supplies every threshold/severity pair, including undefined severities. */
  class AP_R3_LOG_002_FullThresholdMatrix : public LoggingContractFixture,
                                            public testing::WithParamInterface<FilteringMatrixCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify WithLevel and threshold filtering for the full defined level matrix.
   * 1. Arrange: Register a context with the selected threshold and select severity.
   * 2. Act: Submit a non-empty record through WithLevel.
   * 3. Expect: Exact severity output for admitted records, otherwise no metadata or writes.
   */
  TEST_P(AP_R3_LOG_002_FullThresholdMatrix, MatchesConsoleAdmission)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = registry.Register("MATRIX", "Threshold test", parameter.threshold);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    result.Value().get().WithLevel(parameter.severity) << "message";
    /* Expect */
    EXPECT_EQ(provider.reads, parameter.admitted ? 1U : 0U);
    EXPECT_EQ(sink.writes, parameter.admitted ? 1U : 0U);
    if (parameter.admitted)
    {
      EXPECT_EQ(sink.record.View(),
                std::string{"[1970-01-01T00:00:00.000Z][12][34]["} + parameter.label + "][MATRIX] message\n");
    }
    else
    {
      EXPECT_TRUE(sink.record.View().empty());
    }
  }
  std::string FilteringMatrixCaseName(const testing::TestParamInfo<FilteringMatrixCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    AllLevels, AP_R3_LOG_002_FullThresholdMatrix,
    testing::Values(
      FilteringMatrixCase{"Threshold0Severity0", LogLevel::kOff, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold0Severity1", LogLevel::kOff, LogLevel::kFatal, false, "FATAL"},
      FilteringMatrixCase{"Threshold0Severity2", LogLevel::kOff, LogLevel::kError, false, "ERROR"},
      FilteringMatrixCase{"Threshold0Severity3", LogLevel::kOff, LogLevel::kWarn, false, "WARN"},
      FilteringMatrixCase{"Threshold0Severity4", LogLevel::kOff, LogLevel::kInfo, false, "INFO"},
      FilteringMatrixCase{"Threshold0Severity5", LogLevel::kOff, LogLevel::kDebug, false, "DEBUG"},
      FilteringMatrixCase{"Threshold0Severity6", LogLevel::kOff, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold0Severity255", LogLevel::kOff, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold1Severity0", LogLevel::kFatal, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold1Severity1", LogLevel::kFatal, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold1Severity2", LogLevel::kFatal, LogLevel::kError, false, "ERROR"},
      FilteringMatrixCase{"Threshold1Severity3", LogLevel::kFatal, LogLevel::kWarn, false, "WARN"},
      FilteringMatrixCase{"Threshold1Severity4", LogLevel::kFatal, LogLevel::kInfo, false, "INFO"},
      FilteringMatrixCase{"Threshold1Severity5", LogLevel::kFatal, LogLevel::kDebug, false, "DEBUG"},
      FilteringMatrixCase{"Threshold1Severity6", LogLevel::kFatal, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold1Severity255", LogLevel::kFatal, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold2Severity0", LogLevel::kError, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold2Severity1", LogLevel::kError, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold2Severity2", LogLevel::kError, LogLevel::kError, true, "ERROR"},
      FilteringMatrixCase{"Threshold2Severity3", LogLevel::kError, LogLevel::kWarn, false, "WARN"},
      FilteringMatrixCase{"Threshold2Severity4", LogLevel::kError, LogLevel::kInfo, false, "INFO"},
      FilteringMatrixCase{"Threshold2Severity5", LogLevel::kError, LogLevel::kDebug, false, "DEBUG"},
      FilteringMatrixCase{"Threshold2Severity6", LogLevel::kError, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold2Severity255", LogLevel::kError, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold3Severity0", LogLevel::kWarn, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold3Severity1", LogLevel::kWarn, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold3Severity2", LogLevel::kWarn, LogLevel::kError, true, "ERROR"},
      FilteringMatrixCase{"Threshold3Severity3", LogLevel::kWarn, LogLevel::kWarn, true, "WARN"},
      FilteringMatrixCase{"Threshold3Severity4", LogLevel::kWarn, LogLevel::kInfo, false, "INFO"},
      FilteringMatrixCase{"Threshold3Severity5", LogLevel::kWarn, LogLevel::kDebug, false, "DEBUG"},
      FilteringMatrixCase{"Threshold3Severity6", LogLevel::kWarn, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold3Severity255", LogLevel::kWarn, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold4Severity0", LogLevel::kInfo, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold4Severity1", LogLevel::kInfo, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold4Severity2", LogLevel::kInfo, LogLevel::kError, true, "ERROR"},
      FilteringMatrixCase{"Threshold4Severity3", LogLevel::kInfo, LogLevel::kWarn, true, "WARN"},
      FilteringMatrixCase{"Threshold4Severity4", LogLevel::kInfo, LogLevel::kInfo, true, "INFO"},
      FilteringMatrixCase{"Threshold4Severity5", LogLevel::kInfo, LogLevel::kDebug, false, "DEBUG"},
      FilteringMatrixCase{"Threshold4Severity6", LogLevel::kInfo, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold4Severity255", LogLevel::kInfo, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold5Severity0", LogLevel::kDebug, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold5Severity1", LogLevel::kDebug, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold5Severity2", LogLevel::kDebug, LogLevel::kError, true, "ERROR"},
      FilteringMatrixCase{"Threshold5Severity3", LogLevel::kDebug, LogLevel::kWarn, true, "WARN"},
      FilteringMatrixCase{"Threshold5Severity4", LogLevel::kDebug, LogLevel::kInfo, true, "INFO"},
      FilteringMatrixCase{"Threshold5Severity5", LogLevel::kDebug, LogLevel::kDebug, true, "DEBUG"},
      FilteringMatrixCase{"Threshold5Severity6", LogLevel::kDebug, LogLevel::kVerbose, false, "VERBOSE"},
      FilteringMatrixCase{"Threshold5Severity255", LogLevel::kDebug, static_cast<LogLevel>(255), false, "UNDEFINED"},
      FilteringMatrixCase{"Threshold6Severity0", LogLevel::kVerbose, LogLevel::kOff, false, "OFF"},
      FilteringMatrixCase{"Threshold6Severity1", LogLevel::kVerbose, LogLevel::kFatal, true, "FATAL"},
      FilteringMatrixCase{"Threshold6Severity2", LogLevel::kVerbose, LogLevel::kError, true, "ERROR"},
      FilteringMatrixCase{"Threshold6Severity3", LogLevel::kVerbose, LogLevel::kWarn, true, "WARN"},
      FilteringMatrixCase{"Threshold6Severity4", LogLevel::kVerbose, LogLevel::kInfo, true, "INFO"},
      FilteringMatrixCase{"Threshold6Severity5", LogLevel::kVerbose, LogLevel::kDebug, true, "DEBUG"},
      FilteringMatrixCase{"Threshold6Severity6", LogLevel::kVerbose, LogLevel::kVerbose, true, "VERBOSE"},
      FilteringMatrixCase{"Threshold6Severity255", LogLevel::kVerbose, static_cast<LogLevel>(255), false, "UNDEFINED"}),
    FilteringMatrixCaseName);
  /* ============================= End Test_AP_R3_LOG_002 ============================== */
  /* =============================== Test_AP_R3_LOG_009 ================================ */

  struct ConcurrentRecordCase
  {
      const char* name;      // Case description
      bool separate_loggers; // Input separate-context selection
      bool explicit_flush;   // Input flush-and-reuse selection
      std::size_t padding;   // Input payload padding bytes
  };
  void operator<<(std::ostream& out, const ConcurrentRecordCase& value)
  {
    out << value.name;
  }
  /* GetParam selects shared/separate contexts, submission lifecycle, and record size. */
  class AP_R3_LOG_009_CompleteConcurrentRecords : public testing::TestWithParam<ConcurrentRecordCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify coordinated concurrent submissions preserve complete headers and payloads.
   * 1. Arrange: Register contexts, prepare a start gate, and specify each expected payload.
   * 2. Act: Release four waiting workers to submit records by destruction or flush/reuse.
   * 3. Expect: Every complete UTC/PID/TID/context/payload record appears exactly once in any order.
   */
  TEST_P(AP_R3_LOG_009_CompleteConcurrentRecords, PreservesEveryRecord)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    constexpr std::size_t c_workers = 4;
    constexpr int c_records = 16;
    std::array<Logger*, c_workers> loggers{};
    std::array<std::string, c_workers> contexts{};
    std::array<long, c_workers> tids{};
    for (std::size_t worker = 0; worker < c_workers; ++worker)
    {
      contexts[worker] =
        std::string{"CONCURRENT_"} + parameter.name + (parameter.separate_loggers ? std::to_string(worker) : "Shared");
      if (parameter.separate_loggers || worker == 0)
      {
        auto result = ara::log::TryCreateLogger(contexts[worker], "Concurrent header test", LogLevel::kInfo);
        ASSERT_TRUE(result.HasValue());
        loggers[worker] = &result.Value().get();
      }
      else
      {
        loggers[worker] = loggers[0];
      }
    }
    std::mutex gate_mutex;
    std::condition_variable gate;
    std::size_t ready = 0;
    bool start = false;
    std::vector<std::thread> threads;
    threads.reserve(c_workers);
    testing::internal::CaptureStdout();
    /* Act */
    for (std::size_t worker = 0; worker < c_workers; ++worker)
    {
      threads.emplace_back(
        [&, worker]()
        {
          tids[worker] = ::syscall(SYS_gettid);
          {
            std::unique_lock<std::mutex> lock{gate_mutex};
            ++ready;
            gate.notify_all();
            gate.wait(lock,
                      [&]()
                      {
                        return start;
                      });
          }
          if (parameter.explicit_flush)
          {
            auto stream = loggers[worker]->LogInfo();
            for (int index = 0; index < c_records; ++index)
            {
              stream << "worker=" << worker << " record=" << index << ' ' << std::string(parameter.padding, 'x');
              stream.Flush();
              stream.Flush();
            }
          }
          else
          {
            for (int index = 0; index < c_records; ++index)
            {
              loggers[worker]->LogInfo() << "worker=" << worker << " record=" << index << ' '
                                         << std::string(parameter.padding, 'x');
            }
          }
        });
    }
    {
      std::unique_lock<std::mutex> lock{gate_mutex};
      gate.wait(lock,
                [&]()
                {
                  return ready == c_workers;
                });
      start = true;
    }
    gate.notify_all();
    for (auto& thread : threads)
    {
      thread.join();
    }
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), c_workers * c_records);
    std::map<std::string, int> expected;
    for (std::size_t worker = 0; worker < c_workers; ++worker)
    {
      for (int index = 0; index < c_records; ++index)
      {
        const std::string suffix = "[" + std::to_string(::getpid()) + "][" + std::to_string(tids[worker]) + "][INFO][" +
                                   contexts[worker] + "] worker=" + std::to_string(worker) +
                                   " record=" + std::to_string(index) + " " + std::string(parameter.padding, 'x');
        expected.emplace(suffix, 1);
      }
    }
    const std::regex timestamp{R"(\[[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}Z\])"};
    std::size_t begin = 0;
    while (begin < output.size())
    {
      const auto end = output.find('\n', begin);
      ASSERT_NE(end, std::string::npos);
      const std::string line = output.substr(begin, end - begin);
      const auto header_end = line.find(']');
      ASSERT_NE(header_end, std::string::npos);
      EXPECT_TRUE(std::regex_match(line.substr(0, header_end + 1), timestamp));
      EXPECT_EQ(expected.erase(line.substr(header_end + 1)), 1U);
      begin = end + 1;
    }
    EXPECT_TRUE(expected.empty());
  }
  std::string ConcurrentRecordCaseName(const testing::TestParamInfo<ConcurrentRecordCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(SubmissionPaths, AP_R3_LOG_009_CompleteConcurrentRecords,
                           testing::Values(ConcurrentRecordCase{"SharedDestruction", false, false, 64},
                                           ConcurrentRecordCase{"SharedFlush", false, true, 4000},
                                           ConcurrentRecordCase{"SeparateDestruction", true, false, 4000},
                                           ConcurrentRecordCase{"SeparateFlush", true, true, 64}),
                           ConcurrentRecordCaseName);
  /* ============================= End Test_AP_R3_LOG_009 ============================== */
  /* =============================== Test_AP_R3_LOG_008 ================================ */
  /* ----------------------------------------------------------------------------------- */
  /* Verify the full message limit remains usable when console escaping doubles its size.
   * 1. Arrange: Prepare 4096 raw backslash bytes and the expected escaped payload.
   * 2. Act: Submit the maximum-size message by destruction.
   * 3. Expect: One complete record contains all 8192 escaped bytes without truncation.
   */
  TEST_F(LoggingContractFixture, AP_R3_LOG_008_MaximumEscapedMessage)
  {
    /* Arrange */
    constexpr std::size_t c_message_limit = 4096;
    const std::string message(c_message_limit, '\\');
    const std::string expected =
      "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] " + std::string(c_message_limit * 2, '\\') + "\n";
    /* Act */
    logger->LogInfo() << message;
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(), expected);
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify cumulative insertion overflow survives a move and discards the entire pending record.
   * 1. Arrange: Fill the stream to its limit with two separate insertions.
   * 2. Act: Overflow by one byte, move the failed stream, flush it, then submit a fresh record.
   * 3. Expect: The failed record never requests metadata or writes; recovery emits only fresh bytes.
   */
  TEST_F(LoggingContractFixture, AP_R3_LOG_008_MovedOverflowDiscardsWholeRecord)
  {
    /* Arrange */
    constexpr std::size_t c_half_message_limit = 2048;
    auto source = logger->LogInfo();
    source << std::string(c_half_message_limit, 'a') << std::string(c_half_message_limit, 'b');
    /* Act */
    source << 'x';
    auto destination = std::move(source);
    destination << "ignored";
    destination.Flush();
    const auto reads_after_failure = provider.reads;
    const auto writes_after_failure = sink.writes;
    destination << "fresh";
    destination.Flush();
    /* Expect */
    EXPECT_EQ(reads_after_failure, 0U);
    EXPECT_EQ(writes_after_failure, 0U);
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][VALUES] fresh\n");
  }
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
} // namespace
