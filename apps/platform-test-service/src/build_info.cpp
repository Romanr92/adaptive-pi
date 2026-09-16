#include "adaptive_pi/platform_test/build_info.hpp"

namespace adaptive_pi::platform_test
{
  std::string_view ApplicationName() noexcept
  {
    return "platform-test-service";
  }

  std::string_view Version() noexcept
  {
    return "0.2.0";
  }
} // namespace adaptive_pi::platform_test