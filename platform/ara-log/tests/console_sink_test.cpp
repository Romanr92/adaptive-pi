#include "ara/log/logger.h"
#include "console_sink.h"

#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

#ifndef ADAPTIVE_PI_GUNIT_TEST
  #error "Logger sink inspection requires a test-enabled build"
#endif

namespace ara::log::detail
{
  /* Defined only in this test source; friendship permits observation, not replacement. */
  struct LoggerTestAccess
  {
      static Sink& SelectedSink(Logger& logger) noexcept
      {
        return logger.sink_;
      }
  };
} // namespace ara::log::detail

namespace
{
  /* ================================ Test_AP_R3_LOG_004 =============================== */

  /* ----------------------------------------------------------------------------------- */

  /* Verify the production factory selects the concrete console backend.
   * 1. Arrange: Register a unique logger through the public factory.
   * 2. Act: Inspect its selected destination through private test access.
   * 3. Expect: Creation succeeds and the destination is a ConsoleSink.
   */
  TEST(AP_R3_LOG_004_DefaultSinkIsConsole, FactorySelectsConsoleBackend)
  {
    /* Arrange */
    auto result = ara::log::TryCreateLogger("SINK4", "Sink selection", ara::log::LogLevel::kInfo);
    /* Act */
    auto* console = result.HasValue() ? dynamic_cast<ara::log::detail::ConsoleSink*>(
                                          &ara::log::detail::LoggerTestAccess::SelectedSink(result.Value().get()))
                                      : nullptr;
    /* Expect */
    ASSERT_TRUE(result.HasValue());
    EXPECT_NE(console, nullptr);
  }

  /* ----------------------------------------------------------------------------------- */

  /* Verify stdout receives the exact bounded bytes through a created logger's sink.
   * 1. Arrange: Register a logger and prepare bytes including an embedded null.
   * 2. Act: Capture stdout, submit through the selected Sink interface, and collect output.
   * 3. Expect: Submission succeeds and stdout contains every supplied byte exactly once.
   */
  TEST(AP_R3_LOG_004_DefaultSinkIsConsole, SelectedSinkWritesExactBytesToStdout)
  {
    /* Arrange */
    auto result = ara::log::TryCreateLogger("OUT4", "Stdout routing", ara::log::LogLevel::kInfo);
    const std::string payload{"before\0after\n", 13};
    /* Act */
    testing::internal::CaptureStdout();
    auto status = ara::log::detail::SinkStatus::kWriteFailed;
    if (result.HasValue())
    {
      auto& sink = ara::log::detail::LoggerTestAccess::SelectedSink(result.Value().get());
      status = sink.Write(std::string_view{payload});
    }
    const int flush_status = std::fflush(stdout);
    const std::string output = testing::internal::GetCapturedStdout();
    /* Expect */
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(status, ara::log::detail::SinkStatus::kSuccess);
    EXPECT_EQ(flush_status, 0);
    EXPECT_EQ(output, payload);
  }

  /* ============================== End Test_AP_R3_LOG_004 ============================= */
} // namespace
