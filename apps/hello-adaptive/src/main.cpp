#include "ara/core/error_code.h"
#include "ara/core/result.h"
#include "ara/log/logger.h"

#include <cstdlib>
#include <string_view>
#include <tuple>

namespace hello_adaptive
{
  constexpr std::string_view c_context_id{"HELLO"};
  constexpr std::string_view c_context_description{"AdaptivePi hello application"};
  constexpr ara::log::LogLevel c_threshold = ara::log::LogLevel::kInfo;

  using InitializationResult = ara::core::Result<void, ara::core::ErrorCode>;

  /* Uses AP-R3-CORE-011 and AP-R3-LOG-003 */
  [[nodiscard]] InitializationResult Initialize() noexcept
  {
    const auto registration = ara::log::TryCreateLogger(c_context_id, c_context_description, c_threshold);

    if (!registration.HasValue())
    {
      return InitializationResult::FromError(registration.Error());
    }

    return InitializationResult::FromValue();
  }

  /* Uses AP-R3-LOG-010 */
  struct Greeting final
  {
      static constexpr std::string_view kId{"HELLO_GREETING"};
      static constexpr ara::log::LogLevel kLevel = ara::log::LogLevel::kInfo;

      using ParameterTypes = std::tuple<std::string_view>;

      static void Render(ara::log::MessageWriter& writer, const std::string_view& text) noexcept
      {
        writer << text;
      }
  };

} // namespace hello_adaptive

int main()
{
  const auto intitialization = hello_adaptive::Initialize();

  if (!intitialization.HasValue())
  {
    // No registered logger is availabe on this path.
    return EXIT_FAILURE;
  }

  auto& logger = ara::log::CreateLogger(hello_adaptive::c_context_id, hello_adaptive::c_context_description,
                                        hello_adaptive::c_threshold);

  logger.LogInfo() << "Application startup";

  constexpr std::string_view c_greeting{"Hello from AdaptivePi on Embedded Linux."};

  logger.Log(hello_adaptive::Greeting{}, c_greeting);
  return EXIT_SUCCESS;
}