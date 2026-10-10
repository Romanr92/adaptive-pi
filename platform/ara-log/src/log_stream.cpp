#include "ara/log/log_stream.h"

#include "ara/log/logger.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <system_error>
#include <utility>

namespace ara::log
{

  namespace
  {
    constexpr std::size_t c_single_character_size = 1;

    // Enough for max_digits10 significant digits, sign, decimal point,
    // and exponent for float and double on the supported host/ARM64 targets.
    constexpr std::size_t c_floating_buffer_size = 64;

    template <typename T>
    std::to_chars_result FormatFloating(char* begin, char* end, T value) noexcept
    {
      return std::to_chars(begin, end, value, std::chars_format::general, std::numeric_limits<T>::max_digits10);
    }
  } // namespace

  /* Implements AP-R3-LOG-005 */
  LogStream::LogStream(const Logger& logger, LogLevel level) noexcept
      : logger_{&logger}, level_{level}, enabled_{logger.IsEnabled(level)}
  {
  }

  /* Implements AP-R3-LOG-005 */
  void LogStream::Append(std::string_view value) noexcept
  {
    // Disabled, moved-from, or failed streams accumulate no further bytes.
    if ((logger_ == nullptr) || (!enabled_) || (failed_) || (value.empty()))
    {
      return;
    }

    // Subtraction avoids overflow from computing size_ + value.size().
    if (value.size() > (buffer_.size() - size_))
    {
      failed_ = true;
      return;
    }

    // string_view supplies a pointer and length; no null terminator is needed.
    std::memcpy(buffer_.data() + size_, value.data(), value.size());
    size_ += value.size();
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(std::string_view value) noexcept
  {
    Append(value);
    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(const std::string& value) noexcept
  {
    Append(std::string_view(value));
    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(const char* value) noexcept
  {
    // Avoid inspecting input when the stream can't accumulate a record.
    if ((logger_ == nullptr) || (!enabled_) || (failed_))
    {
      return *this;
    }

    // Null is outside the supported input contract; discard safely.
    if (value == nullptr)
    {
      failed_ = true;
      return *this;
    }

    Append(std::string_view{value});
    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(char value) noexcept
  {
    Append(std::string_view{&value, c_single_character_size});
    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(bool value) noexcept
  {
    Append(value ? std::string_view("true") : std::string_view{"false"});
    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(float value) noexcept
  {
    if ((logger_ == nullptr) || (!enabled_) || (failed_))
    {
      return *this;
    }

    if (std::isnan(value))
    {
      Append("nan");
    }
    else if (std::isinf(value))
    {
      Append(std::signbit(value) ? "-inf" : "inf");
    }
    else
    {
      std::array<char, c_floating_buffer_size> text{};
      const auto result = FormatFloating(text.data(), text.data() + text.size(), value);

      if (result.ec != std::errc{})
      {
        failed_ = true;
      }
      else
      {
        const auto length = static_cast<std::size_t>(result.ptr - text.data());

        Append(std::string_view{text.data(), length});
      }
    }

    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream& LogStream::operator<<(double value) noexcept
  {
    if ((logger_ == nullptr) || (!enabled_) || (failed_))
    {
      return *this;
    }

    if (std::isnan(value))
    {
      Append("nan");
    }
    else if (std::isinf(value))
    {
      Append(std::signbit(value) ? "-inf" : "inf");
    }
    else
    {
      std::array<char, c_floating_buffer_size> text{};
      const auto result = FormatFloating(text.data(), text.data() + text.size(), value);

      if (result.ec != std::errc{})
      {
        failed_ = true;
      }
      else
      {
        const auto length = static_cast<std::size_t>(result.ptr - text.data());

        Append(std::string_view{text.data(), length});
      }
    }

    return *this;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream::LogStream(LogStream&& other) noexcept
      : size_{other.size_}, logger_{std::exchange(other.logger_, nullptr)}, level_{other.level_},
        failed_{other.failed_}, enabled_{other.enabled_}
  {
    if (size_ != c_empty_message_size)
    {
      std::memcpy(buffer_.data(), other.buffer_.data(), size_);
    }

    other.size_ = c_empty_message_size;
    other.failed_ = false;
    other.enabled_ = false;
  }

  /* Implements AP-R3-LOG-005 */
  void LogStream::Flush() noexcept
  {
    // Submit only active, enabled, valid streams with accumulated bytes.
    // A moved-from stream has a null logger pointer and cannot submit.
    if ((logger_ != nullptr) && (enabled_) && (!failed_) && (size_ != c_empty_message_size))
    {
      // Borrow exactly size_ bytes without allocation or copying.
      // Submit must consume them before returning.
      logger_->Submit(level_, std::string_view{buffer_.data(), size_});
    }

    // Reset after submission or discard
    size_ = c_empty_message_size;
    failed_ = false;
  }

  /* Implements AP-R3-LOG-005 */
  LogStream::~LogStream() noexcept
  {
    // Submit remaining content.
    Flush();
  }

} // namespace ara::log
