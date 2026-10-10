#include "logger_registry.h"

#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/config.h"
#include "metadata_provider.h"

#include <cstddef>
#include <cstring>
#include <exception>
#include <new>
#include <utility>

namespace ara::log::detail
{
  constexpr std::size_t c_minimum_context_buffer_size = 1;
  constexpr int c_posix_pthread_success = 0;

  /* Implements AP-R3-LOG-003 */
  LoggerCreationResult LoggerRegistry::Register(std::string_view context_id, std::string_view description,
                                                LogLevel threshold) noexcept
  {
    if (static_cast<std::uint8_t>(threshold) > static_cast<std::uint8_t>(LogLevel::kVerbose))
    {
      std::terminate();
    }

    const auto failure = ara::core::MakeErrorCode(ara::core::AdaptivePiErrc::kOperationFailed);

    // nothrow allocation reports failure with nullptr. unique_ptr releases temporary
    // buffers automatically if a later step fails, before publication.
    // Allocate at least one byte so empty views have valid backing storage.
    std::unique_ptr<char[]> id_buffer{
      new (std::nothrow) char[context_id.empty() ? c_minimum_context_buffer_size : context_id.size()]};

    if (!id_buffer)
    {
      return LoggerCreationResult::FromError(failure);
    }

    std::unique_ptr<char[]> description_buffer{
      new (std::nothrow) char[description.empty() ? c_minimum_context_buffer_size : description.size()]};

    if (!description_buffer)
    {
      return LoggerCreationResult::FromError(failure);
    }

    if (!context_id.empty())
    {
      std::memcpy(id_buffer.get(), context_id.data(), context_id.size());
    }

    if (!description.empty())
    {
      std::memcpy(description_buffer.get(), description.data(), description.size());
    }

    // Separate object allocation keeps this address stable as the registry grows.
    // Construction takes ownership of the buffers and performs no allocation.
    Logger* logger = new (std::nothrow) Logger{
      std::move(id_buffer), context_id.size(),          std::move(description_buffer), description.size(), threshold,
      selected_sink_,       selected_metadata_provider_};

    if (logger == nullptr)
    {
      return LoggerCreationResult::FromError(failure);
    }

    if (pthread_mutex_lock(&mutex_) != c_posix_pthread_success)
    {
      delete logger;
      return LoggerCreationResult::FromError(failure);
    }

    // Publish only after successful construction and lock acquisition.
    logger->next_ = head_;
    head_ = logger;

    if (pthread_mutex_unlock(&mutex_) != c_posix_pthread_success)
    {
      std::terminate();
    }

    return LoggerCreationResult::FromValue(std::ref(*logger));
  }

  /* Implements AP-R3-LOG-003 */
  Logger& LoggerRegistry::Find(std::string_view context_id, std::string_view description, LogLevel threshold) noexcept
  {
    if (pthread_mutex_lock(&mutex_) != c_posix_pthread_success)
    {
      std::terminate();
    }

    Logger* found = nullptr;

    for (Logger* current = head_; current != nullptr; current = current->next_)
    {
      if (current->ContextId() == context_id)
      {
        found = current;
        break;
      }
    }

    // && short-circuits: no member access occurs when the search found no object.
    const bool matches =
      ((found != nullptr) && (found->ContextDescription() == description) && (found->DefaultThreshold() == threshold));

    if (pthread_mutex_unlock(&mutex_) != c_posix_pthread_success)
    {
      std::terminate();
    }

    if (!matches)
    {
      std::terminate();
    }

    // Dereferencing returns a borrowed reference, not a copy. No removal occurs
    // during normal operation, so unlocking does not invalidate the object.
    return *found;
  }

  /* Supports AP-R3-LOG-007 */
  LoggerRegistry::LoggerRegistry() noexcept
      : selected_sink_{console_sink_}, selected_metadata_provider_{metadata_provider_}
  {
  }

  /* Supports AP-R3-LOG-007 test metadata injection */
  LoggerRegistry::LoggerRegistry(Sink& sink, MetadataProvider& metadata_provider) noexcept
      : selected_sink_{sink}, selected_metadata_provider_{metadata_provider}
  {
  }

  /* Implements AP-R3-LOG-003 */
  LoggerRegistry::~LoggerRegistry() noexcept
  {
    // Shutdown requires all registry users to have stopped.
    // Save the link before deleting the object that contains it.
    while (head_ != nullptr)
    {
      Logger* current = head_;
      head_ = current->next_;
      delete current;
    }

    if (pthread_mutex_destroy(&mutex_) != c_posix_pthread_success)
    {
      std::terminate();
    }
  }
} // namespace ara::log::detail
