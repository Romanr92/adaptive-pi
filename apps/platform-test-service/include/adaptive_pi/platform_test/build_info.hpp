#ifndef ADAPTIVE_PI_PLATFORM_TEST_BUILD_INFO_HPP
#define ADAPTIVE_PI_PLATFORM_TEST_BUILD_INFO_HPP

#include <string_view>

namespace adaptive_pi::platform_test
{
  [[nodiscard]] std::string_view ApplicationName() noexcept;

  [[nodiscard]] std::string_view Version() noexcept;
} // namespace adaptive_pi::platform_test

#endif // ADAPTIVE_PI_PLATFORM_TEST_BUILD_INFO_HPP