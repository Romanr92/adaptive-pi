#include "ara/log/log_stream.h"
#include "ara/log/logger.h"
#include "console_record_formatter.h"
#include "record_buffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <limits>
#include <ostream>
#include <set>
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
  using ara::log::detail::RecordBuffer;
  using ara::log::detail::RenderConsoleRecord;
  using ara::log::detail::RuntimeMetadata;
  /* =============================== Test_AP_R3_LOG_006 ================================ */
  struct FormatCase
  {
      const char* name;
      std::int64_t timestamp;
      LogLevel level;
      std::string context;
      std::string message;
      std::string expected;
  };
  void operator<<(std::ostream& out, const FormatCase& value)
  {
    out << value.name;
  }
  /* Case data supplies exact console formatting with deterministic supplied metadata; GetParam() selects the scenario.
   */
  class AP_R3_LOG_006_FormatsConsoleRecord : public testing::TestWithParam<FormatCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify exact console formatting with deterministic supplied metadata.
   * 1. Arrange: Prepare an empty buffer and fixed timestamp, PID, TID, context, and message.
   * 2. Act: Render the complete console record using the selected severity.
   * 3. Expect: Require success and compare every rendered byte with the expected record.
   */
  TEST_P(AP_R3_LOG_006_FormatsConsoleRecord, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    RecordBuffer output;
    const RuntimeMetadata metadata{parameter.timestamp, 123, 456};
    /* Act */
    const bool success = RenderConsoleRecord(metadata, parameter.level, parameter.context, parameter.message, output);
    /* Expect */
    EXPECT_TRUE(success);
    EXPECT_EQ(output.View(), parameter.expected);
  }
  std::string FormatCaseName(const testing::TestParamInfo<FormatCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    Cases, AP_R3_LOG_006_FormatsConsoleRecord,
    testing::Values(FormatCase{"Epoch", 0, LogLevel::kInfo, "CTX", "hello",
                               "[1970-01-01T00:00:00.000Z][123][456][INFO][CTX] hello\n"},
                    FormatCase{"NegativeMillisecond", -1, LogLevel::kWarn, "CTX", "hello",
                               "[1969-12-31T23:59:59.999Z][123][456][WARN][CTX] hello\n"},
                    FormatCase{"Milliseconds", 1234, LogLevel::kDebug, "CTX", "hello",
                               "[1970-01-01T00:00:01.234Z][123][456][DEBUG][CTX] hello\n"},
                    FormatCase{"LeapDay", 951827696789LL, LogLevel::kFatal, "CTX", "hello",
                               "[2000-02-29T12:34:56.789Z][123][456][FATAL][CTX] hello\n"},
                    FormatCase{"Escaping", 0, LogLevel::kError, "A[\\]\n\r", "x\\\n\r[y]",
                               "[1970-01-01T00:00:00.000Z][123][456][ERROR][A\\[\\\\\\]\\n\\r] x\\\\\\n\\r[y]\n"},
                    FormatCase{"Verbose", 0, LogLevel::kVerbose, "CTX", "hello",
                               "[1970-01-01T00:00:00.000Z][123][456][VERBOSE][CTX] hello\n"}),
    FormatCaseName);
  /* ============================= End Test_AP_R3_LOG_006 ============================== */
  /* =============================== Test_AP_R3_LOG_008 ================================ */
  struct FailureCase
  {
      const char* name;
      RuntimeMetadata metadata;
      LogLevel level;
      std::string context;
  };
  void operator<<(std::ostream& out, const FailureCase& value)
  {
    out << value.name;
  }
  /* Case data supplies explicit formatting failure status; GetParam() selects the scenario. */
  class AP_R3_LOG_008_FormattingFailureDiscardsRecord : public testing::TestWithParam<FailureCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify explicit formatting failure status.
   * 1. Arrange: Prepare an empty buffer and invalid metadata, severity, or oversized escaped context.
   * 2. Act: Attempt to render a non-empty message.
   * 3. Expect: Require explicit formatting failure.
   */
  TEST_P(AP_R3_LOG_008_FormattingFailureDiscardsRecord, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    RecordBuffer output;
    /* Act */
    const bool success = RenderConsoleRecord(parameter.metadata, parameter.level, parameter.context, "message", output);
    /* Expect */
    EXPECT_FALSE(success);
  }
  std::string FailureCaseName(const testing::TestParamInfo<FailureCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    Cases, AP_R3_LOG_008_FormattingFailureDiscardsRecord,
    testing::Values(FailureCase{"Off", {0, 1, 2}, LogLevel::kOff, "CTX"},
                    FailureCase{"UndefinedLevel", {0, 1, 2}, static_cast<LogLevel>(255), "CTX"},
                    FailureCase{"InvalidPid", {0, 0, 2}, LogLevel::kInfo, "CTX"},
                    FailureCase{"InvalidTid", {0, 1, -1}, LogLevel::kInfo, "CTX"},
                    FailureCase{"YearTooLarge", {253402300800000LL, 1, 2}, LogLevel::kInfo, "CTX"},
                    FailureCase{"EscapedContextOverflow", {0, 1, 2}, LogLevel::kInfo, std::string(8192, '[')}),
    FailureCaseName);
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
  /* =============================== Test_AP_R3_LOG_008 ================================ */
  struct CapacityCase
  {
      const char* name;
      std::size_t bytes;
      bool success;
  };
  void operator<<(std::ostream& out, const CapacityCase& value)
  {
    out << value.name;
  }
  /* Case data supplies bounded append success and rejection without partial copying; GetParam() selects the scenario.
   */
  class AP_R3_LOG_008_RecordBufferCapacity : public testing::TestWithParam<CapacityCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify bounded append success and rejection without partial copying.
   * 1. Arrange: Prepare an empty buffer and an input at the selected capacity boundary.
   * 2. Act: Append the complete input in one operation.
   * 3. Expect: Check success and verify either exact copied bytes or unchanged empty storage.
   */
  TEST_P(AP_R3_LOG_008_RecordBufferCapacity, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    RecordBuffer output;
    const std::string input(parameter.bytes, 'x');
    /* Act */
    const bool success = output.Append(input);
    /* Expect */
    EXPECT_EQ(success, parameter.success);
    EXPECT_EQ(output.View(), parameter.success ? std::string_view{input} : std::string_view{});
  }
  std::string CapacityCaseName(const testing::TestParamInfo<CapacityCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_008_RecordBufferCapacity,
                           testing::Values(CapacityCase{"Empty", 0, true}, CapacityCase{"ExactCapacity", 16384, true},
                                           CapacityCase{"Overflow", 16385, false}),
                           CapacityCaseName);
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
  /* =============================== Test_AP_R3_LOG_005 ================================ */
  struct InsertionCase
  {
      const char* name;
      void (*insert)(ara::log::LogStream&);
      std::string expected;
  };
  void operator<<(std::ostream& out, const InsertionCase& value)
  {
    out << value.name;
  }
  /* Case data supplies supported insertion payloads through a factory-created console logger; GetParam() selects the
   * scenario. */
  class AP_R3_LOG_005_StreamInsertionBuildsRecord : public testing::TestWithParam<InsertionCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify supported insertion payloads through a factory-created console logger.
   * 1. Arrange: Register a verbose-threshold logger with a unique context.
   * 2. Act: Insert the selected values and let the non-empty stream destruct.
   * 3. Expect: Verify the exact severity, context, payload suffix, and one physical newline.
   */
  TEST_P(AP_R3_LOG_005_StreamInsertionBuildsRecord, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"INSERT_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Insertion test", LogLevel::kVerbose);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    testing::internal::CaptureStdout();
    {
      auto stream = result.Value().get().LogInfo();
      parameter.insert(stream);
    }
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    const std::string suffix = std::string{"[INFO]["} + context + "] " + parameter.expected + "\n";
    /* Expect */
    EXPECT_EQ(status, 0);
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  std::string InsertionCaseName(const testing::TestParamInfo<InsertionCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_005_StreamInsertionBuildsRecord,
                           testing::Values(InsertionCase{"StringsAndBool",
                                                         [](auto& s)
                                                         {
                                                           const std::string text{"owned"};
                                                           s << true << ' ' << false << ' ' << "literal " << text << ' '
                                                             << std::string_view{"view"};
                                                         },
                                                         "true false literal owned view"},
                                           InsertionCase{"Integers",
                                                         [](auto& s)
                                                         {
                                                           s << static_cast<signed char>(-12) << ' '
                                                             << static_cast<unsigned char>(255) << ' '
                                                             << std::numeric_limits<long long>::min() << ' '
                                                             << std::numeric_limits<unsigned long long>::max();
                                                         },
                                                         "-12 255 -9223372036854775808 18446744073709551615"},
                                           InsertionCase{"FloatingPrecision",
                                                         [](auto& s)
                                                         {
                                                           s << 0.1F << ' ' << 0.1;
                                                         },
                                                         "0.100000001 0.10000000000000001"},
                                           InsertionCase{"NonFinite",
                                                         [](auto& s)
                                                         {
                                                           s << std::numeric_limits<float>::quiet_NaN() << ' '
                                                             << std::numeric_limits<double>::infinity() << ' '
                                                             << -std::numeric_limits<double>::infinity();
                                                         },
                                                         "nan inf -inf"},
                                           InsertionCase{"NegativeZero",
                                                         [](auto& s)
                                                         {
                                                           s << -0.0;
                                                         },
                                                         "-0"}),
                           InsertionCaseName);
  /* ============================= End Test_AP_R3_LOG_005 ============================== */
  /* =============================== Test_AP_R3_LOG_005 ================================ */
  struct LifecycleCase
  {
      const char* name;
      void (*exercise)(Logger&);
      int records;
  };
  void operator<<(std::ostream& out, const LifecycleCase& value)
  {
    out << value.name;
  }
  /* Case data supplies non-empty flush, destruction, reuse, and move ownership; GetParam() selects the scenario. */
  class AP_R3_LOG_005_FlushAndDestructionSubmitRecord : public testing::TestWithParam<LifecycleCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify non-empty flush, destruction, reuse, and move ownership.
   * 1. Arrange: Register a unique info-threshold logger.
   * 2. Act: Exercise the selected non-empty flush, move, reuse, or destruction sequence.
   * 3. Expect: Verify the exact number and payload of emitted records.
   */
  TEST_P(AP_R3_LOG_005_FlushAndDestructionSubmitRecord, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"LIFE_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Lifecycle test", LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    testing::internal::CaptureStdout();
    parameter.exercise(result.Value().get());
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), parameter.records);
    const std::string marker = std::string{"[INFO]["} + context + "] pending\n";
    std::size_t position = 0;
    for (int index = 0; index < parameter.records; ++index)
    {
      position = output.find(marker, position);
      ASSERT_NE(position, std::string::npos);
      position += marker.size();
    }
  }
  std::string LifecycleCaseName(const testing::TestParamInfo<LifecycleCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_005_FlushAndDestructionSubmitRecord,
                           testing::Values(LifecycleCase{"Destruction",
                                                         [](Logger& l)
                                                         {
                                                           l.LogInfo() << "pending";
                                                         },
                                                         1},
                                           LifecycleCase{"RepeatedFlush",
                                                         [](Logger& l)
                                                         {
                                                           auto s = l.LogInfo();
                                                           s << "pending";
                                                           s.Flush();
                                                           s.Flush();
                                                         },
                                                         1},
                                           LifecycleCase{"Reuse",
                                                         [](Logger& l)
                                                         {
                                                           auto s = l.LogInfo();
                                                           s << "pending";
                                                           s.Flush();
                                                           s << "pending";
                                                         },
                                                         2},
                                           LifecycleCase{"Move",
                                                         [](Logger& l)
                                                         {
                                                           auto source = l.LogInfo();
                                                           source << "pending";
                                                           auto destination = std::move(source);
                                                         },
                                                         1}),
                           LifecycleCaseName);
  /* ============================= End Test_AP_R3_LOG_005 ============================== */
  /* =============================== Test_AP_R3_LOG_005 ================================ */
  struct SeverityCase
  {
      const char* name;
      ara::log::LogStream (Logger::*method)() const noexcept;
      const char* level;
  };
  void operator<<(std::ostream& out, const SeverityCase& value)
  {
    out << value.name;
  }
  /* Case data supplies severity factory methods emitting their configured level; GetParam() selects the scenario. */
  class AP_R3_LOG_005_SeverityMethodsCreateCorrectStreams : public testing::TestWithParam<SeverityCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify severity factory methods emitting their configured level.
   * 1. Arrange: Register a unique logger admitting every defined severity.
   * 2. Act: Invoke the selected severity method and insert a message.
   * 3. Expect: Verify its exact severity and payload with one physical newline.
   */
  TEST_P(AP_R3_LOG_005_SeverityMethodsCreateCorrectStreams, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"SEVERITY_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Severity test", LogLevel::kVerbose);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    testing::internal::CaptureStdout();
    (result.Value().get().*parameter.method)() << "message";
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    const std::string suffix = std::string{"["} + parameter.level + "][" + context + "] message\n";
    /* Expect */
    EXPECT_EQ(status, 0);
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  std::string SeverityCaseName(const testing::TestParamInfo<SeverityCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_005_SeverityMethodsCreateCorrectStreams,
                           testing::Values(SeverityCase{"Fatal", &Logger::LogFatal, "FATAL"},
                                           SeverityCase{"Error", &Logger::LogError, "ERROR"},
                                           SeverityCase{"Warn", &Logger::LogWarn, "WARN"},
                                           SeverityCase{"Info", &Logger::LogInfo, "INFO"},
                                           SeverityCase{"Debug", &Logger::LogDebug, "DEBUG"},
                                           SeverityCase{"Verbose", &Logger::LogVerbose, "VERBOSE"}),
                           SeverityCaseName);
  /* ============================= End Test_AP_R3_LOG_005 ============================== */
  /* =============================== Test_AP_R3_LOG_002 ================================ */
  struct ThresholdCase
  {
      const char* name;
      LogLevel threshold;
      LogLevel severity;
      bool emitted;
  };
  void operator<<(std::ostream& out, const ThresholdCase& value)
  {
    out << value.name;
  }
  /* Case data supplies WithLevel admission and suppression at the console boundary; GetParam() selects the scenario. */
  class AP_R3_LOG_002_EmittedThresholdFiltering : public testing::TestWithParam<ThresholdCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify WithLevel admission and suppression at the console boundary.
   * 1. Arrange: Register a unique logger with the selected threshold.
   * 2. Act: Insert a message through WithLevel using the selected severity.
   * 3. Expect: Verify one record for admitted severities and no output for suppressed ones.
   */
  TEST_P(AP_R3_LOG_002_EmittedThresholdFiltering, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"FILTER_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Filtering test", parameter.threshold);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    testing::internal::CaptureStdout();
    result.Value().get().WithLevel(parameter.severity) << "message";
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(!output.empty(), parameter.emitted);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), parameter.emitted ? 1 : 0);
  }
  std::string ThresholdCaseName(const testing::TestParamInfo<ThresholdCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_002_EmittedThresholdFiltering,
                           testing::Values(ThresholdCase{"AtThreshold", LogLevel::kInfo, LogLevel::kInfo, true},
                                           ThresholdCase{"BelowThreshold", LogLevel::kInfo, LogLevel::kFatal, true},
                                           ThresholdCase{"AboveThreshold", LogLevel::kInfo, LogLevel::kDebug, false},
                                           ThresholdCase{"OffThreshold", LogLevel::kOff, LogLevel::kFatal, false},
                                           ThresholdCase{"OffSeverity", LogLevel::kVerbose, LogLevel::kOff, false},
                                           ThresholdCase{"UndefinedSeverity", LogLevel::kVerbose,
                                                         static_cast<LogLevel>(255), false}),
                           ThresholdCaseName);
  /* ============================= End Test_AP_R3_LOG_002 ============================== */
  /* =============================== Test_AP_R3_LOG_008 ================================ */
  struct OverflowCase
  {
      const char* name;
      std::size_t bytes;
      bool emitted;
  };
  void operator<<(std::ostream& out, const OverflowCase& value)
  {
    out << value.name;
  }
  /* Case data supplies whole-message overflow discard and stream recovery after flush; GetParam() selects the scenario.
   */
  class AP_R3_LOG_008_OverflowDiscardsRecord : public testing::TestWithParam<OverflowCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify whole-message overflow discard and stream recovery after flush.
   * 1. Arrange: Register a logger and prepare a message at or above the 4096-byte limit.
   * 2. Act: Flush that message, then insert a fresh recovery message.
   * 3. Expect: Verify whole-record admission or discard and successful recovery after flush.
   */
  TEST_P(AP_R3_LOG_008_OverflowDiscardsRecord, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    const std::string context = std::string{"OVERFLOW_"} + parameter.name;
    auto result = ara::log::TryCreateLogger(context, "Overflow test", LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const std::string message(parameter.bytes, 'x');
    /* Act */
    testing::internal::CaptureStdout();
    {
      auto stream = result.Value().get().LogInfo();
      stream << message;
      stream.Flush();
      stream << "recovered";
    }
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), parameter.emitted ? 2 : 1);
    EXPECT_NE(output.find("] recovered\n"), std::string::npos);
    EXPECT_EQ(output.find(message) != std::string::npos, parameter.emitted);
  }
  std::string OverflowCaseName(const testing::TestParamInfo<OverflowCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(Cases, AP_R3_LOG_008_OverflowDiscardsRecord,
                           testing::Values(OverflowCase{"ExactMessageLimit", 4096, true},
                                           OverflowCase{"MessageOverflow", 4097, false}),
                           OverflowCaseName);
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
  /* =============================== Test_AP_R3_LOG_007 ================================ */

  /* Verify that a moved stream samples the submitting thread's Linux IDs.
   * 1. Arrange: Create a pending stream on the main thread and capture stdout.
   * 2. Act: Move the stream to a worker and flush it there.
   * 3. Expect: The emitted PID and TID identify the process and worker thread.
   */
  TEST(AP_R3_LOG_007_SubmissionThreadMetadata, UsesSubmittingThread)
  {
    /* Arrange */
    auto result = ara::log::TryCreateLogger("METADATA_WORKER", "Metadata test", LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    auto stream = result.Value().get().LogInfo();
    stream << "moved";
    long submitting_tid = 0;
    testing::internal::CaptureStdout();
    /* Act */
    std::thread worker{[pending = std::move(stream), &submitting_tid]() mutable
                       {
                         submitting_tid = ::syscall(SYS_gettid);
                         pending.Flush();
                       }};
    worker.join();
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_NE(submitting_tid, ::syscall(SYS_gettid));
    const std::string suffix =
      "][" + std::to_string(::getpid()) + "][" + std::to_string(submitting_tid) + "][INFO][METADATA_WORKER] moved\n";
    ASSERT_GE(output.size(), suffix.size());
    EXPECT_EQ(output.substr(output.size() - suffix.size()), suffix);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), 1);
  }
  /* ============================= End Test_AP_R3_LOG_007 ============================== */
  /* =============================== Test_AP_R3_LOG_008 ================================ */

  /* Verify overflow preserves prior bytes and later fitting appends still work.
   * 1. Arrange: Fill the buffer except for one byte.
   * 2. Act: Reject a two-byte append, then append the last byte and an empty value.
   * 3. Expect: Failed append changes nothing and fitting appends succeed.
   */
  TEST(AP_R3_LOG_008_RecordBufferAppend, PreservesContentOnFailure)
  {
    /* Arrange */
    RecordBuffer output;
    const std::string prefix(16383, 'x');
    ASSERT_TRUE(output.Append(prefix));
    /* Act */
    const bool overflow = output.Append("yz");
    const std::string after_failure{output.View()};
    const bool final_byte = output.Append("z");
    const bool empty = output.Append({});
    /* Expect */
    EXPECT_FALSE(overflow);
    EXPECT_EQ(after_failure, prefix);
    EXPECT_TRUE(final_byte);
    EXPECT_TRUE(empty);
    EXPECT_EQ(output.View(), prefix + "z");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify escaped-context overflow discards the complete record at submission.
   * 1. Arrange: Register a context that exceeds rendered capacity after escaping.
   * 2. Act: Submit a non-empty message through its logger.
   * 3. Expect: Stdout receives no partial or complete record.
   */
  TEST(AP_R3_LOG_008_FormattingFailureDiscardsOutput, WritesNothing)
  {
    /* Arrange */
    const std::string context(8192, '[');
    auto result = ara::log::TryCreateLogger(context, "Formatting overflow", LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    testing::internal::CaptureStdout();
    /* Act */
    result.Value().get().LogInfo() << "discarded";
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_TRUE(output.empty());
  }
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
  /* =============================== Test_AP_R3_LOG_009 ================================ */

  /* Verify separate streams sharing one logger emit complete unique records.
   * 1. Arrange: Register one logger and prepare four workers with unique payloads.
   * 2. Act: Emit 32 records per worker concurrently and join every worker.
   * 3. Expect: All 128 payloads occur exactly once without interleaved bytes.
   */
  TEST(AP_R3_LOG_009_ConcurrentRecordsDoNotInterleave, SharedLoggerSeparateStreams)
  {
    /* Arrange */
    constexpr int c_workers = 4;
    constexpr int c_records_per_worker = 32;
    auto result = ara::log::TryCreateLogger("CONCURRENT_RECORDS", "Concurrency test", LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    auto& logger = result.Value().get();
    std::set<std::string> expected;
    for (int worker = 0; worker < c_workers; ++worker)
    {
      for (int record = 0; record < c_records_per_worker; ++record)
      {
        expected.insert("worker=" + std::to_string(worker) + " record=" + std::to_string(record) + " " +
                        std::string(256, 'x'));
      }
    }
    std::vector<std::thread> workers;
    workers.reserve(c_workers);
    testing::internal::CaptureStdout();
    /* Act */
    for (int worker = 0; worker < c_workers; ++worker)
    {
      workers.emplace_back(
        [&logger, worker]()
        {
          for (int record = 0; record < c_records_per_worker; ++record)
          {
            logger.LogInfo() << "worker=" << worker << " record=" << record << " " << std::string(256, 'x');
          }
        });
    }
    for (auto& worker : workers)
    {
      worker.join();
    }
    const int status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, 0);
    EXPECT_EQ(std::count(output.begin(), output.end(), '\n'), c_workers * c_records_per_worker);
    const std::string marker = "[INFO][CONCURRENT_RECORDS] ";
    std::size_t begin = 0;
    while (begin < output.size())
    {
      const auto end = output.find('\n', begin);
      ASSERT_NE(end, std::string::npos);
      const std::string line = output.substr(begin, end - begin);
      const auto payload = line.find(marker);
      ASSERT_NE(payload, std::string::npos);
      EXPECT_EQ(expected.erase(line.substr(payload + marker.size())), 1U);
      begin = end + 1;
    }
    EXPECT_TRUE(expected.empty());
  }
  /* ============================= End Test_AP_R3_LOG_009 ============================== */
} // namespace
