#include "adaptive_pi/platform_test/build_info.hpp"

#include <gtest/gtest.h>

namespace adaptive_pi::platform_test
{
  namespace
  {
    TEST(BuildInfoTest, ReturnsExpectedApplicationName)
    {
      EXPECT_EQ(ApplicationName(), "platform-test-service");
    }

    TEST(BuildInfoTest, ReturnsExpectedVersion)
    {
      EXPECT_EQ(Version(), "0.1.0");
    }
  } // namespace
} // namespace adaptive_pi::platform_test