#ifndef ARA_LOG_LOGGER_H_
#define ARA_LOG_LOGGER_H_

#include "ara/core/error_code.h"
#include "ara/core/result.h"
#include "ara/log/log_level.h"

#include <functional>
#include <string_view>

namespace ara::log
{

  namespace detail
  {
    class LoggerRegistry;
  } // namespace detail

  // AP-R3-LOG-002 and AP-R3-LOG-003
  class Logger final
  {
    public:
      Logger(const Logger&) = delete;
      Logger& operator=(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;

      [[nodiscard]] std::string_view ContextId() const noexcept;
      [[nodiscard]] std::string_view ContextDescription() const noexcept;
      [[nodiscard]] LogLevel DefaultThreshold() const noexcept;

    private:
      friend class detail::LoggerRegistry;

      // Construction and desctruction belong to the framework.
      // Constructor and owned context storage follow next.
      ~Logger() = default;
  };

  // Creates and registers a logger, or returns an error.
  [[nodiscard]] ara::core::Result<std::reference_wrapper<Logger>, ara::core::ErrorCode>
  TryCreateLogger(std::string_view context_id, std::string_view description, LogLevel threshold) noexcept;

  // Retrievees a succesfully registred context with identical inputs.
  [[nodiscard]] Logger& CreateLogger(std::string_view context_id, std::string_view description,
                                     LogLevel threshold) noexcept;

} // namespace ara::log

#endif /* ARA_LOG_LOGGER_H_ */