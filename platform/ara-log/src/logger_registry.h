#ifndef ADAPTIVE_PI_ARA_LOG_LOGGER_REGISTRY_H_
#define ADAPTIVE_PI_ARA_LOG_LOGGER_REGISTRY_H_

#include "ara/log/logger.h"

#include <pthread.h>

namespace ara::log::detail
{

  // A type alias names the common factory result without introducing a new type.
  using LoggerCreationResult = ara::core::Result<std::reference_wrapper<Logger>, ara::core::ErrorCode>;

  // Private framework owner: list insertion never relocates existing logger objects.
  /* Implements AP-R3-LOG-003 */
  class LoggerRegistry final
  {
    public:
      LoggerRegistry() noexcept = default;
      ~LoggerRegistry() noexcept;

      LoggerRegistry(const LoggerRegistry&) = delete;
      LoggerRegistry& operator=(const LoggerRegistry&) = delete;
      LoggerRegistry(LoggerRegistry&&) = delete;
      LoggerRegistry& operator=(LoggerRegistry&&) = delete;

      [[nodiscard]] LoggerCreationResult Register(std::string_view context_id, std::string_view description,
                                                  LogLevel threshold) noexcept;

      [[nodiscard]] Logger& Find(std::string_view context_id, std::string_view description,
                                 LogLevel threshold) noexcept;

    private:
      // POSIX locking reports errors explicitly in both exception configurations.
      // This mutex protects head_ and list traversal; callers must stop before destruction.
      pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
      Logger* head_ = nullptr;
  };

} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_LOGGER_REGISTRY_H_ */