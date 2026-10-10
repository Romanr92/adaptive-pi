#include "ara/core/config.h"
#include "ara/log/log_stream.h"
#include "logger_registry.h"
#include "record_buffer.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <ostream>
#include <string>

namespace
{
  /* ==================================== Fixtures ===================================== */
  class FailureProvider final : public ara::log::detail::MetadataProvider
  {
    public:
      bool fail = false;
      std::size_t reads = 0;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      bool throw_on_read = false;
#endif
      bool Read(ara::log::detail::RuntimeMetadata& metadata) override
      {
        ++reads;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
        if (throw_on_read)
        {
          throw InternalFailure{};
        }
#endif
        if (fail)
        {
          return false;
        }
        metadata = {0, 12, 34};
        return true;
      }
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    private:
      struct InternalFailure
      {
      };
#endif
  };
  class FailureSink final : public ara::log::detail::Sink
  {
    public:
      bool fail = false;
      std::size_t writes = 0;
      ara::log::detail::RecordBuffer records;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      bool throw_internally = false;
      std::size_t contained_exceptions = 0;
#endif
      ara::log::detail::SinkStatus Write(std::string_view record) noexcept override
      {
        ++writes;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
        try
        {
          if (throw_internally)
          {
            throw InternalFailure{};
          }
#endif
          if (fail)
          {
            return ara::log::detail::SinkStatus::kWriteFailed;
          }
          return records.Append(record) ? ara::log::detail::SinkStatus::kSuccess
                                        : ara::log::detail::SinkStatus::kWriteFailed;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
        }
        catch (...)
        {
          ++contained_exceptions;
          return ara::log::detail::SinkStatus::kWriteFailed;
        }
#endif
      }
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    private:
      struct InternalFailure
      {
      };
#endif
  };
  /* Setup registers one logger using fresh controllable dependencies for each test.
   * Destruction releases the registry before its borrowed provider and sink.
   */
  class LoggingFailureFixture : public testing::Test
  {
    protected:
      FailureProvider provider;
      FailureSink sink;
      ara::log::detail::LoggerRegistry registry{sink, provider};
      ara::log::Logger* logger = nullptr;
      void SetUp() override
      {
        auto result = registry.Register("FAILURE", "Failure tests", ara::log::LogLevel::kInfo);
        ASSERT_TRUE(result.HasValue());
        logger = &result.Value().get();
      }
  };
  /* ================================== End Fixtures =================================== */

  /* =============================== Test_AP_R3_LOG_008 ================================ */
  /* ----------------------------------------------------------------------------------- */
  /* Verify explicit provider failure discards flush and destruction records without sink calls.
   * 1. Arrange: Configure the provider to return failure.
   * 2. Act: Flush one record twice and destroy a second pending record.
   * 3. Expect: Both submissions return normally, perform two reads, and write nothing.
   */
  TEST_F(LoggingFailureFixture, AP_R3_LOG_008_ProviderFailureDiscardsRecord)
  {
    /* Arrange */
    provider.fail = true;
    /* Act */
    {
      auto stream = logger->LogInfo();
      stream << "flush failure";
      stream.Flush();
      stream.Flush();
      stream << "destruction failure";
    }
    /* Expect */
    EXPECT_EQ(provider.reads, 2U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_TRUE(sink.records.View().empty());
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify explicit sink failure remains internal for flush and destruction.
   * 1. Arrange: Configure the sink to return kWriteFailed.
   * 2. Act: Flush one record twice and destroy a second pending record.
   * 3. Expect: Submissions return normally without retries, recursion, or recorded bytes.
   */
  TEST_F(LoggingFailureFixture, AP_R3_LOG_008_SinkFailureDoesNotThrow)
  {
    /* Arrange */
    sink.fail = true;
    /* Act */
    {
      auto stream = logger->LogInfo();
      stream << "flush failure";
      stream.Flush();
      stream.Flush();
      stream << "destruction failure";
    }
    /* Expect */
    EXPECT_EQ(provider.reads, 2U);
    EXPECT_EQ(sink.writes, 2U);
    EXPECT_TRUE(sink.records.View().empty());
  }
  /* ----------------------------------------------------------------------------------- */
  struct RecoveryCase
  {
      const char* name;          // Case description
      bool provider_fails;       // Input provider failure flag
      bool sink_fails;           // Input sink failure flag
      std::size_t failed_writes; // Expected writes for discarded record
  };
  void operator<<(std::ostream& output, const RecoveryCase& value)
  {
    output << value.name;
  }
  /* GetParam selects the failing dependency and the expected sink attempt count. */
  class AP_R3_LOG_008_RecoveryAfterFailure : public LoggingFailureFixture,
                                             public testing::WithParamInterface<RecoveryCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify flush discards a failed record and subsequent insertion starts a fresh record.
   * 1. Arrange: Enable the selected provider or sink failure and assemble a message.
   * 2. Act: Flush twice, restore dependencies, insert another message, and flush twice again.
   * 3. Expect: Failure is not retried or replayed; exactly one fresh recovery record is written.
   */
  TEST_P(AP_R3_LOG_008_RecoveryAfterFailure, DiscardsFailedRecordAndResumes)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    provider.fail = parameter.provider_fails;
    sink.fail = parameter.sink_fails;
    auto stream = logger->LogInfo();
    stream << "discarded";
    /* Act */
    stream.Flush();
    stream.Flush();
    const auto failed_reads = provider.reads;
    const auto failed_writes = sink.writes;
    const std::string failed_output{sink.records.View()};
    provider.fail = false;
    sink.fail = false;
    stream << "recovered";
    stream.Flush();
    stream.Flush();
    /* Expect */
    EXPECT_EQ(failed_reads, 1U);
    EXPECT_EQ(failed_writes, parameter.failed_writes);
    EXPECT_TRUE(failed_output.empty());
    EXPECT_EQ(provider.reads, 2U);
    EXPECT_EQ(sink.writes, parameter.failed_writes + 1);
    EXPECT_EQ(sink.records.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][FAILURE] recovered\n");
  }
  std::string RecoveryCaseName(const testing::TestParamInfo<RecoveryCase>& info)
  {
    return info.param.name;
  }
  INSTANTIATE_TEST_SUITE_P(FailedDependencies, AP_R3_LOG_008_RecoveryAfterFailure,
                           testing::Values(RecoveryCase{"Provider", true, false, 0},
                                           RecoveryCase{"Sink", false, true, 1}),
                           RecoveryCaseName);

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
  /* ----------------------------------------------------------------------------------- */
  /* Verify provider exceptions are contained at submission and do not poison the stream.
   * 1. Arrange: Configure the provider to throw during metadata acquisition.
   * 2. Act: Flush a record, destroy another pending record, then restore the provider and submit again.
   * 3. Expect: Failures never reach the sink and a fresh record succeeds after recovery.
   */
  TEST_F(LoggingFailureFixture, AP_R3_LOG_008_ProviderExceptionIsContained)
  {
    /* Arrange */
    provider.throw_on_read = true;
    /* Act */
    {
      auto stream = logger->LogInfo();
      stream << "discarded";
      stream.Flush();
      stream.Flush();
      stream << "also discarded";
    }
    const auto failed_reads = provider.reads;
    const auto failed_writes = sink.writes;
    const std::string failed_output{sink.records.View()};
    provider.throw_on_read = false;
    logger->LogInfo() << "recovered";
    /* Expect */
    EXPECT_EQ(failed_reads, 2U);
    EXPECT_EQ(failed_writes, 0U);
    EXPECT_TRUE(failed_output.empty());
    EXPECT_EQ(provider.reads, 3U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.records.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][FAILURE] recovered\n");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify a sink contains its own exception within noexcept Write and later recovers.
   * 1. Arrange: Configure the private sink to throw and catch an internal failure.
   * 2. Act: Submit a message by destruction, restore the sink, then submit a fresh message.
   * 3. Expect: The caught exception produces failure status, discards the record, and permits recovery.
   */
  TEST_F(LoggingFailureFixture, AP_R3_LOG_008_SinkContainsInternalException)
  {
    /* Arrange */
    sink.throw_internally = true;
    /* Act */
    logger->LogInfo() << "discarded";
    const auto contained = sink.contained_exceptions;
    const auto failed_writes = sink.writes;
    const std::string failed_output{sink.records.View()};
    sink.throw_internally = false;
    logger->LogInfo() << "recovered";
    /* Expect */
    EXPECT_EQ(contained, 1U);
    EXPECT_EQ(failed_writes, 1U);
    EXPECT_TRUE(failed_output.empty());
    EXPECT_EQ(provider.reads, 2U);
    EXPECT_EQ(sink.writes, 2U);
    EXPECT_EQ(sink.contained_exceptions, 1U);
    EXPECT_EQ(sink.records.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][FAILURE] recovered\n");
  }
#endif
  /* ============================= End Test_AP_R3_LOG_008 ============================== */
} // namespace
