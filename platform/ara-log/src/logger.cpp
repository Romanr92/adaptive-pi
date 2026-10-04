#include "ara/log/logger.h"

#include "ara/log/log_stream.h"

#include <exception>
#include <utility>

namespace ara::log
{

  // The initializer list constructs members directly. std::move transfers buffer ownership;
  // it does not allocate or copy the characters.
  /* Implements AP-R3-LOG-002 */
  Logger::Logger(std::unique_ptr<char[]> context_id, std::size_t context_id_size, std::unique_ptr<char[]> description,
                 std::size_t description_size, LogLevel threshold, detail::Sink& sink) noexcept
      : context_id_{std::move(context_id)}, context_id_size_{context_id_size}, description_{std::move(description)},
        description_size_{description_size}, threshold_{threshold}, sink_{sink}
  {
    // An invalid enum value violates the creation contract; terminate does not throw.
    if (static_cast<std::uint8_t>(threshold_) > static_cast<std::uint8_t>(LogLevel::kVerbose))
    {
      std::terminate();
    }
  }

  /* Implements AP-R3-LOG-002 */
  std::string_view Logger::ContextId() const noexcept
  {
    return {context_id_.get(), context_id_size_};
  }

  /* Implements AP-R3-LOG-002 */
  std::string_view Logger::ContextDescription() const noexcept
  {
    return {description_.get(), description_size_};
  }

  /* Implements AP-R3-LOG-002 */
  LogLevel Logger::DefaultThreshold() const noexcept
  {
    return threshold_;
  }

  /* Implements AP-R3-LOG-002 */
  bool Logger::IsEnabled(LogLevel level) const noexcept
  {
    // static_cast explicitly converts the scoped enum for bounded numeric comparison.
    // Off is excluded as a severity; an Off threshold admits no defined severity.
    const auto severity = static_cast<std::uint8_t>(level);
    const auto threshold = static_cast<std::uint8_t>(threshold_);

    return ((severity >= static_cast<std::uint8_t>(LogLevel::kFatal)) &&
            (severity <= static_cast<std::uint8_t>(LogLevel::kVerbose)) &&
            (threshold <= static_cast<std::uint8_t>(LogLevel::kVerbose)) && (severity <= threshold));
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::WithLevel(LogLevel level) const noexcept
  {
    return LogStream{*this, level};
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogFatal() const noexcept
  {
    return WithLevel(LogLevel::kFatal);
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogError() const noexcept
  {
    return WithLevel(LogLevel::kError);
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogWarn() const noexcept
  {
    return WithLevel(LogLevel::kWarn);
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogInfo() const noexcept
  {
    return WithLevel(LogLevel::kInfo);
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogDebug() const noexcept
  {
    return WithLevel(LogLevel::kDebug);
  }

  /* Implements AP-R3-LOG-005 */
  LogStream Logger::LogVerbose() const noexcept
  {
    return WithLevel(LogLevel::kVerbose);
  }

} // namespace ara::log
