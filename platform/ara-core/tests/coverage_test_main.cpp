#include <cstdlib>
#include <exception>
#include <gtest/gtest.h>

/* GCC and Clang's gcov-compatible runtime export this hook in coverage builds.
 * A weak reference leaves ordinary test builds independent of that runtime.
 */
/* Runtime ABI name; it must match the compiler-provided symbol. */
// NOLINTNEXTLINE(bugprone-reserved-identifier)
extern "C" void __gcov_dump() __attribute__((weak));

namespace
{
  std::terminate_handler original_terminate_handler{};

  [[noreturn]] void FlushCoverageAndTerminate() noexcept
  {
    __gcov_dump();
    original_terminate_handler();
    std::abort();
  }
} // namespace

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  /* Death-test children inherit the handler (or install it again on re-exec).
   * Persist their counters before the original handler aborts. Preserve real
   * termination: an ordinary return must still fail every death assertion.
   */
  if (__gcov_dump != nullptr)
  {
    original_terminate_handler = std::set_terminate(FlushCoverageAndTerminate);
  }

  const int result = RUN_ALL_TESTS();
  if (original_terminate_handler != nullptr)
  {
    std::set_terminate(original_terminate_handler);
  }
  return result;
}
