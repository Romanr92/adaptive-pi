#ifndef ARA_LOG_LOGGER_H_
#define ARA_LOG_LOGGER_H_

#include "ara/core/error_code.h"
#include "ara/core/result.h"
#include "ara/log/log_level.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace ara::log
{

  namespace detail
  {
    class LoggerRegistry;
  } // namespace detail

  // final prevents inheritance; the framework controls construction and lifetime.
  /* Implements AP-R3-LOG-002, AP-R3-LOG-003 */
  class Logger final
  {
    public:
      // Deleted special members prevent applications from duplicating or relocating a logger.
      Logger(const Logger&) = delete;
      Logger& operator=(const Logger&) = delete;
      Logger(Logger&&) = delete;
      Logger& operator=(Logger&&) = delete;

      // nodiscard diagnoses ignored results; noexcept promises no escaping exceptions.
      // string_view borrows immutable characters owned by this logger, without copying them.
      [[nodiscard]] std::string_view ContextId() const noexcept;
      [[nodiscard]] std::string_view ContextDescription() const noexcept;
      [[nodiscard]] LogLevel DefaultThreshold() const noexcept;
      [[nodiscard]] bool IsEnabled(LogLevel level) const noexcept;

    private:
      // Friendship gives only this class access to private construction and destruction.
      friend class detail::LoggerRegistry;

      Logger(std::unique_ptr<char[]> context_id, std::size_t context_id_size, std::unique_ptr<char[]> description,
             std::size_t description_size, LogLevel threshold) noexcept;

      ~Logger() = default;

      // Array unique_ptr owns the allocation and automatically calls delete[] at destruction.
      // Lengths preserve bounded string views, including embedded null characters.
      std::unique_ptr<char[]> context_id_;
      std::size_t context_id_size_;
      std::unique_ptr<char[]> description_;
      std::size_t description_size_;
      // const prevents changing the threshold after initialization.
      const LogLevel threshold_;

      // Non-owning list link; the registry owns each node and preserves its address.
      Logger* next_ = nullptr;
  };

  // Result holds either a borrowed reference_wrapper or an ErrorCode.
  // Destroying this result does not destroy the framework-owned logger.
  // Creates and registers a logger, or returns an error.
  [[nodiscard]] ara::core::Result<std::reference_wrapper<Logger>, ara::core::ErrorCode>
  TryCreateLogger(std::string_view context_id, std::string_view description, LogLevel threshold) noexcept;

  // Retrievees a succesfully registred context with identical inputs.
  [[nodiscard]] Logger& CreateLogger(std::string_view context_id, std::string_view description,
                                     LogLevel threshold) noexcept;

} // namespace ara::log

#endif /* ARA_LOG_LOGGER_H_ */