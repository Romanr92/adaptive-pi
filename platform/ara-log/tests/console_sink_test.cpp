#include "console_sink.h"

#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace
{
  /* ================================ Test_AP_R3_LOG_004 =============================== */

  /* ----------------------------------------------------------------------------------- */

  /* Verify the private console backend submits exact bounded bytes to stdout.
   * 1. Arrange: Construct the console backend and prepare bytes including an embedded null.
   * 2. Act: Capture stdout, submit through the Sink interface, and collect output.
   * 3. Expect: Submission succeeds and stdout contains every supplied byte exactly once.
   */
  TEST(AP_R3_LOG_004_DefaultSinkIsConsole, WritesExactBytesToStdout)
  {
    /* Arrange */
    ara::log::detail::ConsoleSink console;
    ara::log::detail::Sink& sink = console;
    const std::string payload{"before\0after\n", 13};
    /* Act */
    testing::internal::CaptureStdout();
    const auto status = sink.Write(std::string_view{payload});
    const int flush_status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    EXPECT_EQ(status, ara::log::detail::SinkStatus::kSuccess);
    EXPECT_EQ(flush_status, 0);
    EXPECT_EQ(output, payload);
  }

  /* ============================== End Test_AP_R3_LOG_004 ============================= */
} // namespace
