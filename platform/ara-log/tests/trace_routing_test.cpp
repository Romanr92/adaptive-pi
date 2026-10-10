#include "ara/core/config.h"
#include "ara/log/logger.h"
#include "logger_registry.h"
#include "record_buffer.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
  #include <stdexcept>
#endif

namespace routing_test
{
  struct TraceCapture
  {
      std::size_t calls = 0;
      std::uint32_t scalar = 0;
      const void* message = nullptr;
      const void* parameter = nullptr;
  };
  template <bool LoggerEnabled, bool TraceEnabled>
  struct Message
  {
      static constexpr std::string_view kId{"ROUTE"};
      static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
      using ParameterTypes = std::tuple<std::string_view>;
      TraceCapture* capture;
      std::size_t* renders;
      void Render(ara::log::MessageWriter& writer, const std::string_view& text) const noexcept
      {
        ++*renders;
        writer << text;
      }
  };
  struct ScalarMessage
  {
      static constexpr std::string_view kId{"SCALAR"};
      static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
      using ParameterTypes = std::tuple<std::uint32_t>;
      TraceCapture* capture;
      static void Render(ara::log::MessageWriter& writer, const std::uint32_t& value) noexcept
      {
        writer << value;
      }
  };
} // namespace routing_test
namespace ara::log
{
  template <bool L, bool T>
  struct TraceRouting<routing_test::Message<L, T>>
  {
      static constexpr bool kLoggerEnabled = L;
      static constexpr bool kTraceEnabled = T;
  };
  template <>
  struct TraceRouting<routing_test::ScalarMessage>
  {
      static constexpr bool kLoggerEnabled = true;
      static constexpr bool kTraceEnabled = true;
  };
} // namespace ara::log
namespace ara::log::ext
{
  template <>
  void
  TraceArti<routing_test::Message<false, true>, std::string_view>(const routing_test::Message<false, true>& message,
                                                                  const std::string_view& text) noexcept
  {
    ++message.capture->calls;
    message.capture->message = &message;
    message.capture->parameter = &text;
  }
  template <>
  void TraceArti<routing_test::Message<true, true>, std::string_view>(const routing_test::Message<true, true>& message,
                                                                      const std::string_view& text) noexcept
  {
    ++message.capture->calls;
    message.capture->message = &message;
    message.capture->parameter = &text;
  }
  template <>
  void TraceArti<routing_test::ScalarMessage, std::uint32_t>(const routing_test::ScalarMessage& message,
                                                             const std::uint32_t& value) noexcept
  {
    ++message.capture->calls;
    message.capture->scalar = value;
  }
} // namespace ara::log::ext
namespace
{
  /* ==================================== Fixtures ===================================== */

  class RoutingProvider final : public ara::log::detail::MetadataProvider
  {
    public:
      std::size_t reads = 0;
      bool fail = false;
      bool throw_failure = false;
      bool Read(ara::log::detail::RuntimeMetadata& metadata) override
      {
        ++reads;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
        if (throw_failure)
        {
          throw std::runtime_error{"Injected metadata failure"};
        }
#endif
        metadata = {0, 12, 34};
        return !fail;
      }
  };
  class RoutingSink final : public ara::log::detail::Sink
  {
    public:
      std::size_t writes = 0;
      bool fail = false;
      ara::log::detail::RecordBuffer record;
      ara::log::detail::SinkStatus Write(std::string_view text) noexcept override
      {
        ++writes;
        if (fail)
        {
          return ara::log::detail::SinkStatus::kWriteFailed;
        }
        return record.Append(text) ? ara::log::detail::SinkStatus::kSuccess
                                   : ara::log::detail::SinkStatus::kWriteFailed;
      }
  };
  /* Fresh dependencies and registry isolate each test; cleanup destroys registered loggers
   * before their borrowed dependencies. Capture state records original argument identities. */
  class RoutingFixture : public testing::Test
  {
    protected:
      RoutingProvider provider;
      RoutingSink sink;
      ara::log::detail::LoggerRegistry registry{sink, provider};
      routing_test::TraceCapture capture;
      std::size_t renders = 0;
  };
  /* ================================== End Fixtures =================================== */
  /* =============================== Test_AP_R3_LOG_011 ================================ */
  /* ----------------------------------------------------------------------------------- */
  /* Verify the configured destinations and original trace arguments.
   * 1. Arrange: Register a logger and prepare a message with the selected compile-time route.
   * 2. Act: Submit the modelled message through LogWith with a Tag.
   * 3. Expect: Only enabled destinations run and logger output contains one complete record.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_RoutesToLogger)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Routing tests", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<true, false> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().LogWith(std::make_tuple(ara::log::Tag{"test"}), message, text);
    /* Expect */
    EXPECT_EQ(renders, 1U);
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(capture.calls, 0U);
    EXPECT_EQ(capture.message, nullptr);
    EXPECT_EQ(capture.parameter, nullptr);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][ROUTING] id=ROUTE ready tag=\"test\"\n");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify the configured destinations and original trace arguments.
   * 1. Arrange: Register a logger and prepare a message with the selected compile-time route.
   * 2. Act: Submit the modelled message through LogWith with a Tag.
   * 3. Expect: Only enabled destinations run and logger output contains one complete record.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_RoutesToTraceArtifact)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Routing tests", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<false, true> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().LogWith(std::make_tuple(ara::log::Tag{"test"}), message, text);
    /* Expect */
    EXPECT_EQ(renders, 0U);
    EXPECT_EQ(provider.reads, 0U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.message, &message);
    EXPECT_EQ(capture.parameter, &text);
    EXPECT_EQ(sink.record.View(), "");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify the configured destinations and original trace arguments.
   * 1. Arrange: Register a logger and prepare a message with the selected compile-time route.
   * 2. Act: Submit the modelled message through LogWith with a Tag.
   * 3. Expect: Only enabled destinations run and logger output contains one complete record.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_RoutesToBoth)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Routing tests", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<true, true> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().LogWith(std::make_tuple(ara::log::Tag{"test"}), message, text);
    /* Expect */
    EXPECT_EQ(renders, 1U);
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.message, &message);
    EXPECT_EQ(capture.parameter, &text);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][ROUTING] id=ROUTE ready tag=\"test\"\n");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify the configured destinations and original trace arguments.
   * 1. Arrange: Register a logger and prepare a message with the selected compile-time route.
   * 2. Act: Submit the modelled message through LogWith with a Tag.
   * 3. Expect: Only enabled destinations run and logger output contains one complete record.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_DiscardsDisabledTraceMessage)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Routing tests", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<false, false> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().LogWith(std::make_tuple(ara::log::Tag{"test"}), message, text);
    /* Expect */
    EXPECT_EQ(renders, 0U);
    EXPECT_EQ(provider.reads, 0U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_EQ(capture.calls, 0U);
    EXPECT_EQ(capture.message, nullptr);
    EXPECT_EQ(capture.parameter, nullptr);
    EXPECT_EQ(sink.record.View(), "");
  }

  struct RoutingFailureCase
  {
      const char* name;             // Case description
      ara::log::LogLevel threshold; // Input context threshold
      bool provider_failure;        // Input metadata failure
      bool sink_failure;            // Input write failure
      bool overflow;                // Input oversized payload
      bool throw_provider;          // Input exception injection in enabled CI
      std::size_t expected_reads;   // Expected metadata acquisitions
      std::size_t expected_writes;  // Expected sink attempts
      std::size_t expected_renders; // Expected renderer invocations
  };
  void operator<<(std::ostream& out, const RoutingFailureCase& value)
  {
    out << value.name;
  }
  /* GetParam chooses filtering or a logger failure while keeping both destinations enabled. */
  class AP_R3_LOG_011_LoggerFailureDoesNotSuppressTrace : public RoutingFixture,
                                                          public testing::WithParamInterface<RoutingFailureCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify logger filtering and failure cannot suppress configured trace delivery.
   * 1. Arrange: Register the selected threshold and inject the case's logger failure.
   * 2. Act: Submit the original message and text through Log.
   * 3. Expect: Exactly one trace receives original arguments despite no console record.
   */
  TEST_P(AP_R3_LOG_011_LoggerFailureDoesNotSuppressTrace, PreservesTraceInvocation)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    provider.fail = parameter.provider_failure;
    provider.throw_failure = parameter.throw_provider;
    sink.fail = parameter.sink_failure;
    auto result = registry.Register("ROUTING", "Failure routing", parameter.threshold);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<true, true> message{&capture, &renders};
    constexpr std::size_t c_message_limit = 4096;
    const std::string storage(parameter.overflow ? c_message_limit + 1 : 1, 'x');
    const std::string_view text{storage};
    /* Act */
    result.Value().get().Log(message, text);
    /* Expect */
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.message, &message);
    EXPECT_EQ(capture.parameter, &text);
    EXPECT_EQ(renders, parameter.expected_renders);
    EXPECT_EQ(provider.reads, parameter.expected_reads);
    EXPECT_EQ(sink.writes, parameter.expected_writes);
    EXPECT_TRUE(sink.record.View().empty());
  }
  std::string RoutingFailureCaseName(const testing::TestParamInfo<RoutingFailureCase>& info)
  {
    return info.param.name;
  }
  std::vector<RoutingFailureCase> RoutingFailureCases()
  {
    return {
      {"Off", ara::log::LogLevel::kOff, false, false, false, false, 0, 0, 0},
      {"Filtered", ara::log::LogLevel::kWarn, false, false, false, false, 0, 0, 0},
      {"ProviderFailure", ara::log::LogLevel::kInfo, true, false, false, false, 1, 0, 1},
      {"SinkFailure", ara::log::LogLevel::kInfo, false, true, false, false, 1, 1, 1},
      {"Overflow", ara::log::LogLevel::kInfo, false, false, true, false, 0, 0, 1},
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      {"ProviderException", ara::log::LogLevel::kInfo, false, false, false, true, 1, 0, 1},
#endif
    };
  }
  INSTANTIATE_TEST_SUITE_P(IndependentTrace, AP_R3_LOG_011_LoggerFailureDoesNotSuppressTrace,
                           testing::ValuesIn(RoutingFailureCases()), RoutingFailureCaseName);

  /* ----------------------------------------------------------------------------------- */
  /* Verify trace-only routing ignores an Off threshold.
   * 1. Arrange: Register an Off logger and prepare a trace-only message.
   * 2. Act: Submit the original message and parameter.
   * 3. Expect: Trace receives both original arguments once; rendering and providers stay idle.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_TraceOnlyAtOff)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Trace only Off", ara::log::LogLevel::kOff);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<false, true> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().Log(message, text);
    /* Expect */
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.message, &message);
    EXPECT_EQ(capture.parameter, &text);
    EXPECT_EQ(renders, 0U);
    EXPECT_EQ(provider.reads, 0U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_TRUE(sink.record.View().empty());
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify console-formatting overflow cannot suppress trace delivery.
   * 1. Arrange: Register a context larger than the console record buffer with both routes.
   * 2. Act: Submit a short valid modelled payload.
   * 3. Expect: Metadata is sampled but no sink write occurs; original arguments reach trace.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_FormattingFailurePreservesTrace)
  {
    /* Arrange */
    constexpr std::size_t c_console_capacity = 16384;
    const std::string context(c_console_capacity + 1, 'x');
    auto result = registry.Register(context, "Formatting failure", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const routing_test::Message<true, true> message{&capture, &renders};
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().Log(message, text);
    /* Expect */
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.message, &message);
    EXPECT_EQ(capture.parameter, &text);
    EXPECT_EQ(renders, 1U);
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 0U);
    EXPECT_TRUE(sink.record.View().empty());
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify volatile scalars reach the declared normalized trace specialization.
   * 1. Arrange: Register a logger and define only the uint32_t trace specialization.
   * 2. Act: Log a volatile scalar through both enabled destinations.
   * 3. Expect: Linking succeeds, the trace records its value, and console rendering agrees.
   */
  TEST_F(RoutingFixture, AP_R3_LOG_011_VolatileScalarUsesNormalizedTrace)
  {
    /* Arrange */
    auto result = registry.Register("ROUTING", "Volatile trace", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    volatile std::uint32_t value = 7;
    const routing_test::ScalarMessage message{&capture};
    /* Act */
    result.Value().get().Log(message, value);
    /* Expect */
    EXPECT_EQ(capture.calls, 1U);
    EXPECT_EQ(capture.scalar, 7U);
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(), "[1970-01-01T00:00:00.000Z][12][34][INFO][ROUTING] id=SCALAR 7\n");
  }
  /* ============================= End Test_AP_R3_LOG_011 ============================== */
} // namespace
