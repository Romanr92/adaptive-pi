#include "adaptive_pi/platform_test/build_info.hpp"

#include <gtest/gtest.h>

namespace adaptive_pi::platform_test
{
  namespace
  {
    /* ----------------------------------------------------------------------------------- */

    /* Verify the reported application name.
     * 1. Arrange: Define the expected build metadata.
     * 2. Act: Call ApplicationName().
     * 3. Expect: The returned metadata matches the expected value.
     */
    TEST(BuildInfoTest, ReturnsExpectedApplicationName)
    {
      /* Arrange */
      const auto expected = "platform-test-service";

      /* Act */
      const auto actual = ApplicationName();

      /* Expect */
      EXPECT_EQ(actual, expected);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify the reported version.
     * 1. Arrange: Define the expected build metadata.
     * 2. Act: Call Version().
     * 3. Expect: The returned metadata matches the expected value.
     */
    TEST(BuildInfoTest, ReturnsExpectedVersion)
    {
      /* Arrange */
      const auto expected = "0.2.0";

      /* Act */
      const auto actual = Version();

      /* Expect */
      EXPECT_EQ(actual, expected);
    }
  } /* namespace */
} /* namespace adaptive_pi::platform_test */