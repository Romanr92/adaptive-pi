#ifndef ARA_LOG_LOG_STREAM_H_
#define ARA_LOG_LOG_STREAM_H_

#include "ara/log/log_level.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace ara::log
{

  namespace
  {
    static constexpr int c_integer_overload_tag = 0;
  } // namespace

  class Logger;

  /* Implements AP-R3-LOG-005 */
  class LogStream final
  {
    public:
      LogStream(const LogStream&) = delete;
      LogStream& operator=(const LogStream&) = delete;

      // Transfers responsibility for submitting the pending record.
      LogStream(LogStream&& other) noexcept;
      LogStream& operator=(LogStream&&) = delete;

      // Submits or discards the pending record without throwing.
      ~LogStream() noexcept;

      // Starts a fresh logical record after submission or discard.
      void Flush() noexcept;

      /* Implements AP-R3-LOG-005 */
      LogStream& operator<<(std::string_view value) noexcept;
      LogStream& operator<<(const std::string& value) noexcept;
      LogStream& operator<<(const char* value) noexcept;
      LogStream& operator<<(char value) noexcept;
      LogStream& operator<<(bool value) noexcept;
      LogStream& operator<<(float value) noexcept;
      LogStream& operator<<(double value) noexcept;

      /* Implements AP-R3-LOG-005 */
      template <typename T, std::enable_if_t<(std::is_integral_v<T>) && (!std::is_same_v<T, bool>) &&
                                               (!std::is_same_v<T, char>) && (!std::is_same_v<T, wchar_t>) &&
                                               (!std::is_same_v<T, char16_t>) && (!std::is_same_v<T, char32_t>),
                                             int> = c_integer_overload_tag>
      LogStream& operator<<(T value) noexcept
      {
        if ((logger_ == nullptr) || (!enabled_) || (failed_))
        {
          return *this;
        }

        // digits10 counts guaranteed decimal digits.
        // Two extra positions allow the remaining digit and a minus sign.
        constexpr std::size_t c_integer_format_margin = 2;
        constexpr std::size_t c_integer_buffer_size = std::numeric_limits<T>::digits10 + c_integer_format_margin;

        std::array<char, c_integer_buffer_size> text{};

        const auto result = std::to_chars(text.data(), text.data() + text.size(), value);

        if (result.ec != std::errc{})
        {
          failed_ = true;
          return *this;
        }

        const auto length = static_cast<std::size_t>(result.ptr - text.data());

        Append(std::string_view{text.data(), length});
        return *this;
      }

      /* Implements AP-R3-LOG-005 */
      template <
        typename T,
        std::enable_if_t<
          !(((std::is_integral_v<std::decay_t<T>>) && (!std::is_same_v<std::decay_t<T>, wchar_t>) &&
             (!std::is_same_v<std::decay_t<T>, char16_t>) && (!std::is_same_v<std::decay_t<T>, char32_t>)) ||
            (std::is_same_v<std::decay_t<T>, float>) || (std::is_same_v<std::decay_t<T>, double>) ||
            (std::is_same_v<std::decay_t<T>, std::string>) || (std::is_same_v<std::decay_t<T>, std::string_view>) ||
            (std::is_same_v<std::decay_t<T>, char*>) || (std::is_same_v<std::decay_t<T>, const char*>)),
          int> = c_integer_overload_tag>
      LogStream& operator<<(T&& value) = delete;

    private:
      // Only Logger creates streams with a destination and severity.
      friend class Logger;

      LogStream(const Logger& logger, LogLevel level) noexcept;

      // Appends bytes only when the pending record remains vaild.
      void Append(std::string_view value) noexcept;

      // LOG-008 establishes this limit; no heal allocattion is needed.
      static constexpr std::size_t c_message_capacity = 4096;
      static constexpr std::size_t c_empty_message_size = 0;

      std::array<char, c_message_capacity> buffer_{};
      std::size_t size_ = c_empty_message_size;

      // Borrowed Logger; nullptr marks an inactive moved-from stream.
      const Logger* logger_;
      LogLevel level_;

      // Overflow or formatting failure invalidates the whole pending record.
      bool failed_ = false;

      // Disabled streams avoid message assembly.
      bool enabled_;
  };

} // namespace ara::log

#endif /* ARA_LOG_LOG_STREAM_H_ */