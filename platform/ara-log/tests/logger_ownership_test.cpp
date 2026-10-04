#include "ara/log/logger.h"

#include <array>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <type_traits>

namespace
{
  using ara::log::Logger;
  using ara::log::LogLevel;
  /* ================================ Test_AP_R3_LOG_003 =============================== */

  static_assert(!std::is_default_constructible_v<Logger>);
  static_assert(!std::is_copy_constructible_v<Logger>);
  static_assert(!std::is_move_constructible_v<Logger>);
  static_assert(!std::is_copy_assignable_v<Logger>);
  static_assert(!std::is_move_assignable_v<Logger>);
  static_assert(!std::is_destructible_v<Logger>);

  /* ----------------------------------------------------------------------------------- */

  /* Verify framework ownership and reference stability after registry growth.
   * 1. Arrange: Register a unique context and retain its address.
   * 2. Act: Destroy the factory result and register more distinct contexts.
   * 3. Expect: Lookup returns the original address with intact context data.
   */
  TEST(AP_R3_LOG_003_CreateLoggerOwnsLogger, ReferencesRemainStable)
  {
    /* Arrange */
    Logger* original = nullptr;
    {
      auto result = ara::log::TryCreateLogger("OWN3", "Original", LogLevel::kInfo);
      ASSERT_TRUE(result.HasValue());
      original = &result.Value().get();
    }
    /* Act */
    auto second = ara::log::TryCreateLogger("NEW3A", "Second", LogLevel::kDebug);
    auto third = ara::log::TryCreateLogger("NEW3B", "Third", LogLevel::kWarn);
    auto& retrieved = ara::log::CreateLogger("OWN3", "Original", LogLevel::kInfo);
    /* Expect */
    ASSERT_TRUE(second.HasValue());
    ASSERT_TRUE(third.HasValue());
    EXPECT_EQ(&retrieved, original);
    EXPECT_EQ(original->ContextId(), "OWN3");
    EXPECT_EQ(original->ContextDescription(), "Original");
    EXPECT_EQ(original->DefaultThreshold(), LogLevel::kInfo);
    EXPECT_NE(original, &second.Value().get());
    EXPECT_NE(original, &third.Value().get());
  }

  /* ----------------------------------------------------------------------------------- */

  /* Verify concurrent registration and lookup with caller-unique IDs.
   * 1. Arrange: Prepare distinct IDs and independent per-thread result slots.
   * 2. Act: Release all workers together, register and retrieve each context, then join.
   * 3. Expect: Every worker gets its own stable logger with the supplied properties.
   */
  TEST(AP_R3_LOG_003_ConcurrentCreation, RegistersAndRetrievesUniqueContexts)
  {
    /* Arrange */
    constexpr std::size_t count = 8;
    const std::array<const char*, count> ids{"C30", "C31", "C32", "C33", "C34", "C35", "C36", "C37"};
    std::array<Logger*, count> created{};
    std::array<Logger*, count> retrieved{};
    std::array<std::thread, count> workers;
    std::atomic<bool> start{false};
    /* Act */
    for (std::size_t i = 0; i < count; ++i)
    {
      workers[i] = std::thread(
        [&, i]
        {
          while (!start.load())
          {
            std::this_thread::yield();
          }
          auto result = ara::log::TryCreateLogger(ids[i], "Concurrent", LogLevel::kInfo);
          if (result.HasValue())
          {
            created[i] = &result.Value().get();
            retrieved[i] = &ara::log::CreateLogger(ids[i], "Concurrent", LogLevel::kInfo);
          }
        });
    }
    start.store(true);
    for (auto& worker : workers)
    {
      worker.join();
    }
    /* Expect */
    for (std::size_t i = 0; i < count; ++i)
    {
      ASSERT_NE(created[i], nullptr);
      EXPECT_EQ(created[i], retrieved[i]);
      EXPECT_EQ(created[i]->ContextId(), ids[i]);
      EXPECT_EQ(created[i]->ContextDescription(), "Concurrent");
      EXPECT_EQ(created[i]->DefaultThreshold(), LogLevel::kInfo);
      for (std::size_t j = 0; j < i; ++j)
      {
        EXPECT_NE(created[i], created[j]);
      }
    }
  }

  /* ----------------------------------------------------------------------------------- */

  /* Verify the missing-registration precondition.
   * 1. Arrange: Select an ID never registered by this test executable.
   * 2. Act: Request its reference in an isolated death-test process.
   * 3. Expect: The process terminates rather than returning a fallback.
   */
  TEST(AP_R3_LOG_003_LookupPreconditions, MissingContextTerminates)
  {
    /* Arrange */
    const char* id = "MISSING3";
    /* Act */
    const auto retrieve = [id]
    {
      (void)ara::log::CreateLogger(id, "Missing", LogLevel::kInfo);
    };
    /* Expect */
    EXPECT_DEATH(retrieve(), "");
  }

  /* ----------------------------------------------------------------------------------- */

  /* Verify both independent matching preconditions for an existing ID.
   * 1. Arrange: Successfully register a unique context.
   * 2. Act: Prepare lookups with a different description or threshold.
   * 3. Expect: Each mismatched lookup terminates in an isolated process.
   */
  TEST(AP_R3_LOG_003_LookupPreconditions, MismatchedInputsTerminate)
  {
    /* Arrange */
    auto result = ara::log::TryCreateLogger("MATCH3", "Expected", LogLevel::kInfo);
    /* Act */
    const auto wrong_description = []
    {
      (void)ara::log::CreateLogger("MATCH3", "Wrong", LogLevel::kInfo);
    };
    const auto wrong_threshold = []
    {
      (void)ara::log::CreateLogger("MATCH3", "Expected", LogLevel::kDebug);
    };
    /* Expect */
    ASSERT_TRUE(result.HasValue());
    EXPECT_DEATH(wrong_description(), "");
    EXPECT_DEATH(wrong_threshold(), "");
  }
  /* ============================== End Test_AP_R3_LOG_003 ============================= */

} // namespace
