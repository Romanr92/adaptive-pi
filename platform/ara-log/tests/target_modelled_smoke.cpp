#include "ara/log/logger.h"

#include <cstdint>
#include <string_view>
#include <tuple>

namespace target_smoke
{
  inline std::uint32_t trace_calls = 0;
  template <bool LoggerEnabled, bool TraceEnabled>
  struct Message
  {
      static constexpr std::string_view kId{"SMOKE"};
      static constexpr ara::log::LogLevel kLevel{ara::log::LogLevel::kInfo};
      using ParameterTypes = std::tuple<std::uint32_t>;
      static void Render(ara::log::MessageWriter& writer, const std::uint32_t& value) noexcept
      {
        writer << value;
      }
  };
} // namespace target_smoke
namespace ara::log
{
  template <bool L, bool T>
  struct TraceRouting<target_smoke::Message<L, T>>
  {
      static constexpr bool kLoggerEnabled = L;
      static constexpr bool kTraceEnabled = T;
  };
} // namespace ara::log
namespace ara::log::ext
{
  template <>
  void TraceArti<target_smoke::Message<false, true>, std::uint32_t>(const target_smoke::Message<false, true>&,
                                                                    const std::uint32_t&) noexcept
  {
    ++target_smoke::trace_calls;
  }
  template <>
  void TraceArti<target_smoke::Message<true, true>, std::uint32_t>(const target_smoke::Message<true, true>&,
                                                                   const std::uint32_t&) noexcept
  {
    ++target_smoke::trace_calls;
  }
} // namespace ara::log::ext
/* AP-R3-LOG-010 and AP-R3-LOG-011 target compile/link verification.
 * 1. Arrange: Register a context and define all four routes with real trace specializations.
 * 2. Act: Instantiate Log and LogWith, including both typed attributes.
 * 3. Expect: The executable links for the target; native execution observes two trace calls.
 * CI checks the AArch64 ELF header without executing the target binary.
 */
int main()
{
  auto result = ara::log::TryCreateLogger("SMOKE", "Target modelled API verification", ara::log::LogLevel::kInfo);
  if (!result.HasValue())
  {
    return 1;
  }
  auto& logger = result.Value().get();
  constexpr std::uint32_t value = 2;
  logger.Log(target_smoke::Message<true, false>{}, value);
  logger.Log(target_smoke::Message<false, true>{}, value);
  logger.LogWith(std::make_tuple(ara::log::Location{"smoke.cpp", 1}, ara::log::Tag{"target"}),
                 target_smoke::Message<true, true>{}, value);
  logger.Log(target_smoke::Message<false, false>{}, value);
  return target_smoke::trace_calls == value ? 0 : 1;
}
