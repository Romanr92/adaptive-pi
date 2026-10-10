#include "ara/log/logger.h"
#include "logger_registry.h"
#include "record_buffer.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>

namespace
{
  /* ==================================== Fixtures ===================================== */
  class ModelledMetadata final : public ara::log::detail::MetadataProvider
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
  class ModelledSink final : public ara::log::detail::Sink
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
  /* Each test owns a fresh registry and deterministic dependencies; destruction releases
   * registered loggers before their borrowed sink and provider are destroyed. */
  class ModelledFixture : public testing::Test
  {
    protected:
      ModelledMetadata provider;
      ModelledSink sink;
      ara::log::detail::LoggerRegistry registry{sink, provider};
  };
  /* ================================== End Fixtures =================================== */
  /* =============================== Test_AP_R3_LOG_010 ================================ */
  struct TextMessage
  {
      static constexpr std::string_view kId{"TEXT"};
      static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
      using ParameterTypes = std::tuple<std::string_view>;
      static void Render(ara::log::MessageWriter& writer, const std::string_view& text) noexcept
      {
        writer << text;
      }
  };
  static_assert(ara::log::detail::kParameterMatch<TextMessage::ParameterTypes, const std::string_view&>);
  static_assert(!ara::log::detail::kParameterMatch<TextMessage::ParameterTypes, std::string>);
  static_assert(!ara::log::detail::kParameterMatch<TextMessage::ParameterTypes>);
  static_assert(!ara::log::detail::kValidAttributes<ara::log::Tag, ara::log::Tag>);
  static_assert(!ara::log::detail::kValidAttributes<int>);

  struct ModelledPayloadCase
  {
      const char* name;             // Case description
      std::string text;             // Input rendered text
      ara::log::LogLevel threshold; // Input context threshold
      std::string expected;         // Expected console payload, or empty for discard
  };
  void operator<<(std::ostream& out, const ModelledPayloadCase& value)
  {
    out << value.name;
  }
  /* GetParam supplies text, threshold, and expected payload for boundary and suppression cases. */
  class AP_R3_LOG_010_LogsModelledMessage : public ModelledFixture,
                                            public testing::WithParamInterface<ModelledPayloadCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify complete modelled payloads and whole-record suppression.
   * 1. Arrange: Register a logger with the case threshold and prepare its text.
   * 2. Act: Log the text through the modelled API.
   * 3. Expect: Exact output is emitted once, or no metadata and output occur.
   */
  TEST_P(AP_R3_LOG_010_LogsModelledMessage, MatchesContract)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = registry.Register("MODEL", "Modelled tests", parameter.threshold);
    ASSERT_TRUE(result.HasValue());
    const std::string_view text{parameter.text};
    /* Act */
    result.Value().get().Log(TextMessage{}, text);
    /* Expect */
    const bool emitted = !parameter.expected.empty();
    EXPECT_EQ(provider.reads, emitted ? 1U : 0U);
    EXPECT_EQ(sink.writes, emitted ? 1U : 0U);
    const std::string expected =
      emitted ? "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] " + parameter.expected + "\n" : "";
    EXPECT_EQ(sink.record.View(), expected);
  }
  std::string ModelledPayloadCaseName(const testing::TestParamInfo<ModelledPayloadCase>& info)
  {
    return info.param.name;
  }
  constexpr std::size_t c_message_limit = 4096;
  constexpr std::size_t c_text_prefix_size = std::string_view{"id=TEXT "}.size();
  INSTANTIATE_TEST_SUITE_P(
    Payloads, AP_R3_LOG_010_LogsModelledMessage,
    testing::Values(ModelledPayloadCase{"Text", "ready", ara::log::LogLevel::kInfo, "id=TEXT ready"},
                    ModelledPayloadCase{"Empty", "", ara::log::LogLevel::kInfo, "id=TEXT"},
                    ModelledPayloadCase{"Filtered", "ready", ara::log::LogLevel::kWarn, ""},
                    ModelledPayloadCase{"Off", "ready", ara::log::LogLevel::kOff, ""},
                    ModelledPayloadCase{"ExactLimit", std::string(c_message_limit - c_text_prefix_size, 'x'),
                                        ara::log::LogLevel::kInfo,
                                        "id=TEXT " + std::string(c_message_limit - c_text_prefix_size, 'x')},
                    ModelledPayloadCase{"Overflow", std::string(c_message_limit - c_text_prefix_size + 1, 'x'),
                                        ara::log::LogLevel::kInfo, ""}),
    ModelledPayloadCaseName);
  /* ----------------------------------------------------------------------------------- */
  /* Verify attribute tuple order and both attribute and console escaping.
   * 1. Arrange: Register a logger and supply Tag before Location with special characters.
   * 2. Act: Submit a modelled message with both attributes.
   * 3. Expect: One exact record preserves order and applies each escaping stage.
   */
  TEST_F(ModelledFixture, AP_R3_LOG_010_LogsModelledMessageWithAttributes)
  {
    /* Arrange */
    auto result = registry.Register("MODEL", "Attribute tests", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const auto attributes = std::make_tuple(ara::log::Tag{"a\"b\\c\n"}, ara::log::Location{"file.cpp", 42});
    constexpr std::string_view text{"ready"};
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, text);
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    constexpr std::string_view expected =
      R"([1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] id=TEXT ready tag="a\\"b\\\\c\n" location="file.cpp:42")"
      "\n";
    EXPECT_EQ(sink.record.View(), expected);
  }

  struct AttributeBoundaryCase
  {
      const char* name;     // Case description
      std::string tag;      // Input tag value
      std::string expected; // Expected escaped payload, or empty for discard
  };
  void operator<<(std::ostream& out, const AttributeBoundaryCase& value)
  {
    out << value.name;
  }
  /* GetParam supplies empty, escaped, and boundary-sized tags with exact expected payloads. */
  class AP_R3_LOG_010_ModelledMessageSizeLimit : public ModelledFixture,
                                                 public testing::WithParamInterface<AttributeBoundaryCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify attribute bytes and escaping expansion share the message size limit.
   * 1. Arrange: Register a logger and build the case tag with empty rendered text.
   * 2. Act: Submit the attribute, then submit a fresh modelled record.
   * 3. Expect: The attribute record is exact or wholly discarded; the fresh record succeeds.
   */
  TEST_P(AP_R3_LOG_010_ModelledMessageSizeLimit, AttributesShareLimitAndRecover)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = registry.Register("MODEL", "Attribute limits", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const auto attributes = std::make_tuple(ara::log::Tag{parameter.tag});
    constexpr std::string_view empty{};
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, empty);
    const auto attribute_reads = provider.reads;
    const auto attribute_writes = sink.writes;
    result.Value().get().Log(TextMessage{}, empty);
    /* Expect */
    const bool emitted = !parameter.expected.empty();
    EXPECT_EQ(attribute_reads, emitted ? 1U : 0U);
    EXPECT_EQ(attribute_writes, emitted ? 1U : 0U);
    EXPECT_EQ(provider.reads, attribute_reads + 1U);
    EXPECT_EQ(sink.writes, attribute_writes + 1U);
    constexpr std::string_view header{"[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] "};
    const std::string expected =
      (emitted ? std::string{header} + parameter.expected + "\n" : "") + std::string{header} + "id=TEXT\n";
    EXPECT_EQ(sink.record.View(), expected);
  }
  std::string AttributeBoundaryCaseName(const testing::TestParamInfo<AttributeBoundaryCase>& info)
  {
    return info.param.name;
  }
  constexpr std::size_t c_tag_overhead = std::string_view{"id=TEXT tag=\"\""}.size();
  constexpr std::size_t c_tag_capacity = c_message_limit - c_tag_overhead;
  INSTANTIATE_TEST_SUITE_P(
    AttributeBounds, AP_R3_LOG_010_ModelledMessageSizeLimit,
    testing::Values(AttributeBoundaryCase{"Empty", "", "id=TEXT tag=\"\""},
                    AttributeBoundaryCase{"Plain", "ready", "id=TEXT tag=\"ready\""},
                    AttributeBoundaryCase{"ExactLimit", std::string(c_tag_capacity, 'x'),
                                          "id=TEXT tag=\"" + std::string(c_tag_capacity, 'x') + "\""},
                    AttributeBoundaryCase{"Overflow", std::string(c_tag_capacity + 1, 'x'), ""},
                    AttributeBoundaryCase{"EscapeOverflow", std::string(c_tag_capacity, '\\'), ""}),
    AttributeBoundaryCaseName);

  struct ValueMessage
  {
      static constexpr std::string_view kId{"VALUES_01"};
      static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
      using ParameterTypes = std::tuple<>;
      static void Render(ara::log::MessageWriter& writer) noexcept
      {
        const std::string empty_string;
        constexpr std::string_view empty_view{};
        constexpr const char* empty_c_string = "";
        constexpr const char* c_string = "cstring";
        const std::string string{"string"};
        writer << empty_string << empty_view << empty_c_string;
        writer << true << '|' << false << '|' << char{'X'} << '|' << std::numeric_limits<std::int64_t>::min() << '|'
               << std::numeric_limits<std::uint64_t>::max() << '|' << float{1.5F} << '|' << double{2.25} << '|'
               << std::numeric_limits<double>::quiet_NaN() << '|' << std::numeric_limits<float>::infinity() << '|'
               << -std::numeric_limits<double>::infinity() << '|' << c_string << '|' << string << '|'
               << std::string_view{"view"};
      }
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify supported value categories and the separator after initial empty insertions.
   * 1. Arrange: Register a deterministic logger and a zero-parameter value renderer.
   * 2. Act: Log the message containing empty inputs followed by multiple typed insertions.
   * 3. Expect: One exact record renders boundaries and non-finite values with one separator.
   */
  TEST_F(ModelledFixture, AP_R3_LOG_010_SupportedRenderingValues)
  {
    /* Arrange */
    auto result = registry.Register("MODEL", "Value rendering", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    /* Act */
    result.Value().get().Log(ValueMessage{});
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    EXPECT_EQ(sink.record.View(),
              "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] id=VALUES_01 "
              "true|false|X|-9223372036854775808|18446744073709551615|1.5|2.25|nan|inf|-inf|cstring|string|view\n");
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify LocationOnlyEscapesFilename with exact attribute and console escaping.
   * 1. Arrange: Register a logger and prepare an escaped filename and the required tuple.
   * 2. Act: Submit that tuple with non-empty message text.
   * 3. Expect: One exact record preserves attribute order and escapes the filename.
   */
  TEST_F(ModelledFixture, AP_R3_LOG_010_LocationOnlyEscapesFilename)
  {
    /* Arrange */
    auto result = registry.Register("MODEL", "Attribute order", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const ara::log::Location location{"a\"b\\c\r", 0};
    constexpr std::string_view text{"message"};
    const auto attributes = std::make_tuple(location);
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, text);
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    constexpr std::string_view expected = "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] "
                                          R"(id=TEXT message location="a\\"b\\\\c\r:0")"
                                          "\n";
    EXPECT_EQ(sink.record.View(), expected);
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify LocationThenTagPreservesOrder with exact attribute and console escaping.
   * 1. Arrange: Register a logger and prepare an escaped filename and the required tuple.
   * 2. Act: Submit that tuple with non-empty message text.
   * 3. Expect: One exact record preserves attribute order and escapes the filename.
   */
  TEST_F(ModelledFixture, AP_R3_LOG_010_LocationThenTagPreservesOrder)
  {
    /* Arrange */
    auto result = registry.Register("MODEL", "Attribute order", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const ara::log::Location location{"a\"b\\c\r", 0};
    const ara::log::Tag tag{"ready"};
    constexpr std::string_view text{"message"};
    const auto attributes = std::make_tuple(location, tag);
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, text);
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    constexpr std::string_view expected = "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] "
                                          R"(id=TEXT message location="a\\"b\\\\c\r:0" tag="ready")"
                                          "\n";
    EXPECT_EQ(sink.record.View(), expected);
  }
  /* ----------------------------------------------------------------------------------- */
  /* Verify TagThenLocationPreservesOrder with exact attribute and console escaping.
   * 1. Arrange: Register a logger and prepare an escaped filename and the required tuple.
   * 2. Act: Submit that tuple with non-empty message text.
   * 3. Expect: One exact record preserves attribute order and escapes the filename.
   */
  TEST_F(ModelledFixture, AP_R3_LOG_010_TagThenLocationPreservesOrder)
  {
    /* Arrange */
    auto result = registry.Register("MODEL", "Attribute order", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const ara::log::Location location{"a\"b\\c\r", 0};
    const ara::log::Tag tag{"ready"};
    constexpr std::string_view text{"message"};
    const auto attributes = std::make_tuple(tag, location);
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, text);
    /* Expect */
    EXPECT_EQ(provider.reads, 1U);
    EXPECT_EQ(sink.writes, 1U);
    constexpr std::string_view expected = "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] "
                                          R"(id=TEXT message tag="ready" location="a\\"b\\\\c\r:0")"
                                          "\n";
    EXPECT_EQ(sink.record.View(), expected);
  }
  struct CombinedLimitCase
  {
      const char* name;      // Case description
      std::size_t text_size; // Input message text bytes
      bool emitted;          // Expected record admission
  };
  void operator<<(std::ostream& out, const CombinedLimitCase& value)
  {
    out << value.name;
  }
  /* GetParam varies text length around the combined ID, text, and two-attribute limit. */
  class AP_R3_LOG_010_CombinedMessageLimit : public ModelledFixture,
                                             public testing::WithParamInterface<CombinedLimitCase>
  {
  };
  /* ----------------------------------------------------------------------------------- */
  /* Verify non-empty text and both attributes share one raw payload limit.
   * 1. Arrange: Prepare a boundary-sized text, Location, and Tag with known overhead.
   * 2. Act: Submit the combined record.
   * 3. Expect: Exact-capacity records are complete and one-byte overflow requests no metadata.
   */
  TEST_P(AP_R3_LOG_010_CombinedMessageLimit, IncludesTextAndBothAttributes)
  {
    /* Arrange */
    const auto& parameter = GetParam();
    auto result = registry.Register("MODEL", "Combined limit", ara::log::LogLevel::kInfo);
    ASSERT_TRUE(result.HasValue());
    const std::string text(parameter.text_size, 'x');
    const auto attributes = std::make_tuple(ara::log::Location{"f", 1}, ara::log::Tag{"t"});
    /* Act */
    result.Value().get().LogWith(attributes, TextMessage{}, std::string_view{text});
    /* Expect */
    EXPECT_EQ(provider.reads, parameter.emitted ? 1U : 0U);
    EXPECT_EQ(sink.writes, parameter.emitted ? 1U : 0U);
    const std::string expected = parameter.emitted ? "[1970-01-01T00:00:00.000Z][12][34][INFO][MODEL] id=TEXT " + text +
                                                       " location=\"f:1\" tag=\"t\"\n"
                                                   : "";
    EXPECT_EQ(sink.record.View(), expected);
  }
  std::string CombinedLimitCaseName(const testing::TestParamInfo<CombinedLimitCase>& info)
  {
    return info.param.name;
  }
  constexpr std::size_t c_combined_overhead = std::string_view{"id=TEXT  location=\"f:1\" tag=\"t\""}.size();
  INSTANTIATE_TEST_SUITE_P(CombinedBounds, AP_R3_LOG_010_CombinedMessageLimit,
                           testing::Values(CombinedLimitCase{"ExactLimit", c_message_limit - c_combined_overhead, true},
                                           CombinedLimitCase{"Overflow", c_message_limit - c_combined_overhead + 1,
                                                             false}),
                           CombinedLimitCaseName);
  /* ============================= End Test_AP_R3_LOG_010 ============================== */
} // namespace
