#include "ara/log/log_stream.h"
#include "logger_registry.h"
#include "record_buffer.h"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <utility>

namespace
{
  using ara::log::LogLevel;
  using ara::log::detail::RuntimeMetadata;

  /* ==================================== Fixtures ===================================== */
  class StubMetadataProvider final : public ara::log::detail::MetadataProvider
  {
    public:
      RuntimeMetadata value{0, 12, 34};
      std::size_t reads = 0;
      bool Read(RuntimeMetadata& output) noexcept override
      {
        ++reads;
        output = value;
        return true;
      }
  };
  class RecordingSink final : public ara::log::detail::Sink
  {
    public:
      ara::log::detail::RecordBuffer records;
      std::size_t writes = 0;
      ara::log::detail::SinkStatus Write(std::string_view record) noexcept override
      {
        ++writes;
        return records.Append(record) ? ara::log::detail::SinkStatus::kSuccess
                                      : ara::log::detail::SinkStatus::kWriteFailed;
      }
  };
  /* Each test receives fresh dependencies and a registry borrowing them.
   * Setup registers a verbose logger; destruction releases the registry before its dependencies.
   */
  class MetadataFixture : public testing::Test
  {
    protected:
      StubMetadataProvider provider;
      RecordingSink sink;
      ara::log::detail::LoggerRegistry registry{sink, provider};
      ara::log::Logger* logger = nullptr;
      void SetUp() override
      {
        auto result = registry.Register("STUB", "Metadata fixture", LogLevel::kVerbose);
        ASSERT_TRUE(result.HasValue());
        logger = &result.Value().get();
      }
  };
  /* ================================== End Fixtures =================================== */

  /* =============================== Test_AP_R3_LOG_007 ================================ */
  struct StubMetadataCase
  {
      const char* name;         // Case description
      RuntimeMetadata metadata; // Input timestamp in milliseconds, PID, and TID
      const char* expected;     // Expected complete console record
  };
  void operator<<(std::ostream& output, const StubMetadataCase& value)
  {
    output << value.name;
  }
  /* GetParam supplies controlled metadata and the exact expected record for the real submission path. */
  class AP_R3_LOG_007_FormatsStubbedRuntimeMetadata : public MetadataFixture,
                                                      public testing::WithParamInterface<StubMetadataCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify injected metadata reaches the rendered sink record unchanged.
   * 1. Arrange: Set the provider to the selected timestamp, PID, and TID.
   * 2. Act: Submit a message by destroying a non-empty stream.
   * 3. Expect: The sink receives exact bytes after one provider read and one write.
   */
  TEST_P(AP_R3_LOG_007_FormatsStubbedRuntimeMetadata, UsesControlledProvider)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    provider.value = parameter.metadata;
    /* Act */
    logger->LogInfo() << "message";
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.records.View(), parameter.expected);
  }
  std::string StubMetadataCaseName(const testing::TestParamInfo<StubMetadataCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    ControlledMetadata, AP_R3_LOG_007_FormatsStubbedRuntimeMetadata,
    testing::Values(
      StubMetadataCase{"Epoch", {0, 12, 34}, "[1970-01-01T00:00:00.000Z][12][34][INFO][STUB] message\n"},
      StubMetadataCase{"Fraction", {1234, 56, 78}, "[1970-01-01T00:00:01.234Z][56][78][INFO][STUB] message\n"},
      StubMetadataCase{"BeforeEpoch", {-1, 90, 123}, "[1969-12-31T23:59:59.999Z][90][123][INFO][STUB] message\n"}),
    StubMetadataCaseName);

  /* ----------------------------------------------------------------------------------- */
  /* Verify each logical record samples metadata when submitted, not when assembled.
   * 1. Arrange: Assemble a message with the provider set to epoch metadata.
   * 2. Act: Change metadata before flush, then change it again before destruction of a second record.
   * 3. Expect: No early read occurs; each record contains its own submission-time values.
   */
  TEST_F(MetadataFixture, AP_R3_LOG_007_SamplesEachSubmission)
  {
    /* Arrange */
    auto stream = logger->LogInfo();
    stream << "first";
    const auto reads_before_flush = provider.reads;
    /* Act */
    provider.value = {1001, 20, 30};
    stream.Flush();
    const auto reads_after_flush = provider.reads;
    stream << "second";
    const auto reads_after_insertion = provider.reads;
    provider.value = {2002, 40, 50};
    {
      auto pending = std::move(stream);
    }
    /* Expect */
    EXPECT_EQ(reads_before_flush, 0U);
    EXPECT_EQ(reads_after_flush, 1U);
    EXPECT_EQ(reads_after_insertion, 1U);
    EXPECT_EQ(provider.reads, 2U);
    EXPECT_EQ(sink.writes, 2U);
    EXPECT_EQ(sink.records.View(), "[1970-01-01T00:00:01.001Z][20][30][INFO][STUB] first\n"
                                   "[1970-01-01T00:00:02.002Z][40][50][INFO][STUB] second\n");
  }

  /* ----------------------------------------------------------------------------------- */
  struct SuppressedMetadataCase
  {
      const char* name;    // Case description
      LogLevel threshold;  // Input context threshold
      LogLevel severity;   // Input message severity
      const char* message; // Input empty or filtered message
  };
  void operator<<(std::ostream& output, const SuppressedMetadataCase& value)
  {
    output << value.name;
  }
  /* GetParam selects empty, off, undefined, or above-threshold records whose dependencies must remain untouched. */
  class AP_R3_LOG_007_SuppressedRecordsSkipProviders : public MetadataFixture,
                                                       public testing::WithParamInterface<SuppressedMetadataCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify suppressed records neither acquire metadata nor write to the sink.
   * 1. Arrange: Register a logger using the selected threshold.
   * 2. Act: Insert the selected message, flush twice, and destroy the stream.
   * 3. Expect: Provider and sink call counts remain zero and no bytes are recorded.
   */
  TEST_P(AP_R3_LOG_007_SuppressedRecordsSkipProviders, SkipsReadAndWrite)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = registry.Register("SUPPRESSED", "Suppression test", parameter.threshold);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    {
      auto stream = result.Value().get().WithLevel(parameter.severity);
      stream << parameter.message;
      stream.Flush();
      stream.Flush();
    }
    /* Expect */
    EXPECT_EQ(provider.reads, 0U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_TRUE(sink.records.View().empty());
  }
  std::string SuppressedMetadataCaseName(const testing::TestParamInfo<SuppressedMetadataCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(
    SuppressedMetadata, AP_R3_LOG_007_SuppressedRecordsSkipProviders,
    testing::Values(SuppressedMetadataCase{"Empty", LogLevel::kVerbose, LogLevel::kInfo, ""},
                    SuppressedMetadataCase{"AboveThreshold", LogLevel::kInfo, LogLevel::kDebug, "message"},
                    SuppressedMetadataCase{"OffThreshold", LogLevel::kOff, LogLevel::kFatal, "message"},
                    SuppressedMetadataCase{"OffSeverity", LogLevel::kVerbose, LogLevel::kOff, "message"},
                    SuppressedMetadataCase{"UndefinedSeverity", LogLevel::kVerbose, static_cast<LogLevel>(255),
                                           "message"}),
    SuppressedMetadataCaseName);
  /* ============================= End Test_AP_R3_LOG_007 ============================== */
} // namespace
