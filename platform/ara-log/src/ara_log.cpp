#include "ara/log/logger.h"
#include "logger_registry.h"

namespace ara::log
{

  namespace
  {
    // Internal linkage keeps this helper private to this translation unit.
    detail::LoggerRegistry& GetLoggerRegistry() noexcept
    {
      // Initialized once on first use. C++11 and later synchronize initialization.
      // Its destructor runs during normal process shutdown.
      static detail::LoggerRegistry registry;
      return registry;
    }
  } // namespace

  /* Implements AP-R3-LOG-003 */
  ara::core::Result<std::reference_wrapper<Logger>, ara::core::ErrorCode>
  TryCreateLogger(std::string_view context_id, std::string_view description, LogLevel threshold) noexcept
  {
    return GetLoggerRegistry().Register(context_id, description, threshold);
  }

  /* Implements AP-R3-LOG-003 */
  Logger& CreateLogger(std::string_view context_id, std::string_view description, LogLevel threshold) noexcept
  {
    return GetLoggerRegistry().Find(context_id, description, threshold);
  }

} // namespace ara::log