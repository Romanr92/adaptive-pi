#include "ara/log/logger.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

struct NumericMessage
{
    static constexpr std::string_view kId{"NUMBER"};
    static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
    using ParameterTypes = std::tuple<std::uint32_t>;
    static void Render(ara::log::MessageWriter& writer, const std::uint32_t& value) noexcept
    {
      writer << value;
    }
};
struct StringMessage
{
    static constexpr std::string_view kId{"STRING"};
    static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
    using ParameterTypes = std::tuple<std::string_view>;
    static void Render(ara::log::MessageWriter& writer, const std::string_view& value) noexcept
    {
      writer << value;
    }
};
struct PairMessage
{
    static constexpr std::string_view kId{"PAIR_01"};
    static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
    using ParameterTypes = std::tuple<const std::uint32_t&, std::string_view>;
    static void Render(ara::log::MessageWriter& writer, const std::uint32_t& value,
                       const std::string_view& text) noexcept
    {
      writer << value << text;
    }
};
struct InvalidIdMessage : NumericMessage
{
#if MODELLED_CASE == 9
    static constexpr std::string_view kId{""};
#elif MODELLED_CASE == 10
    static constexpr std::string_view kId{"BAD-ID"};
#else
    static constexpr std::string_view kId{"BAD\x80"};
#endif
};
struct UnsupportedRenderMessage : NumericMessage
{
    static void Render(ara::log::MessageWriter& writer, const std::uint32_t&) noexcept
    {
#if MODELLED_CASE == 13
      writer << static_cast<const void*>(nullptr);
#elif MODELLED_CASE == 14
      writer << L'X';
#endif
    }
};
/* Instantiated calls verify the public API, rather than only its helper traits.
 * 1. Arrange: Select a compiler case and an existing borrowed logger.
 * 2. Act: Compile the selected call without linking.
 * 3. Expect: Valid controls compile; invalid calls fail with the required diagnostic.
 */
void Check(ara::log::Logger& logger)
{
#if MODELLED_CASE == 0
  const std::uint32_t number = 2;
  logger.Log(NumericMessage{}, number);
  logger.LogWith(std::make_tuple(ara::log::Location{"test.cpp", 1}, ara::log::Tag{"test"}), NumericMessage{}, number);
  logger.Log(StringMessage{}, std::string_view{"test"});
  logger.Log(PairMessage{}, number, std::string_view{"test"});
#elif MODELLED_CASE == 1
  logger.Log(NumericMessage{}, 2.0);
#elif MODELLED_CASE == 2
  logger.Log(NumericMessage{});
#elif MODELLED_CASE == 3
  logger.Log(NumericMessage{}, std::uint32_t{2}, std::uint32_t{3});
#elif MODELLED_CASE == 4
  logger.Log(StringMessage{}, std::string{"test"});
#elif MODELLED_CASE == 5
  logger.Log(StringMessage{}, "test");
#elif MODELLED_CASE == 6
  logger.LogWith(std::make_tuple(42), NumericMessage{}, std::uint32_t{2});
#elif MODELLED_CASE == 7
  logger.LogWith(std::make_tuple(ara::log::Tag{"a"}, ara::log::Tag{"b"}), NumericMessage{}, std::uint32_t{2});
#elif MODELLED_CASE == 8
  logger.LogWith(std::make_tuple(ara::log::Location{"a", 1}, ara::log::Location{"b", 2}), NumericMessage{},
                 std::uint32_t{2});
#elif MODELLED_CASE == 9 || MODELLED_CASE == 10 || MODELLED_CASE == 11
  logger.Log(InvalidIdMessage{}, std::uint32_t{2});
#elif MODELLED_CASE == 12
  logger.Log(PairMessage{}, std::string_view{"test"}, std::uint32_t{2});
#elif MODELLED_CASE == 13 || MODELLED_CASE == 14
  logger.Log(UnsupportedRenderMessage{}, std::uint32_t{2});
#elif MODELLED_CASE == 15
  volatile std::uint32_t value = 2;
  logger.Log(NumericMessage{}, value);
  logger.LogWith(std::make_tuple(ara::log::Tag{"volatile"}), NumericMessage{}, value);
#elif MODELLED_CASE == 16
  const volatile std::uint32_t value = 3;
  logger.Log(NumericMessage{}, value);
  logger.Log(PairMessage{}, value, std::string_view{"test"});
#endif
}
