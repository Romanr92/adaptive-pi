#ifndef ARA_LOG_MESSAGE_WRITER_H_
#define ARA_LOG_MESSAGE_WRITER_H_

#include "ara/log/log_stream.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ara::log
{
  class Logger;

  /* Supports AP-R3-LOG-010 */
  class MessageWriter final
  {
    public:
      MessageWriter(const MessageWriter&) = delete;
      MessageWriter& operator=(const MessageWriter&) = delete;
      MessageWriter(MessageWriter&&) = delete;
      MessageWriter& operator=(MessageWriter&&) = delete;

      template <typename T>
      auto operator<<(T&& value) noexcept
        -> decltype(std::declval<LogStream&>() << std::forward<T>(value), std::declval<MessageWriter&>())
      {
        using ValueType = std::decay_t<T>;

        // Empty text must not introduce a separator after the message ID.
        if constexpr (std::is_same_v<ValueType, std::string> || std::is_same_v<ValueType, std::string_view>)
        {
          if (value.empty())
          {
            return *this;
          }
        }
        else if constexpr (std::is_same_v<ValueType, char*> || std::is_same_v<ValueType, const char*>)
        {
          if (value == nullptr)
          {
            // LogStream marks the whole pending record as failed.
            stream_ << value;
            return *this;
          }

          if (*value == '\0')
          {
            return *this;
          }
        }

        if (!text_started_)
        {
          stream_ << c_text_separator;
          text_started_ = true;
        }

        stream_ << std::forward<T>(value);
        return *this;
      }

    private:
      friend class Logger;

      explicit MessageWriter(LogStream& stream) noexcept : stream_{stream} {}

      static constexpr std::string_view c_text_separator{" "};

      LogStream& stream_;
      bool text_started_ = false;
  };

} // namespace ara::log

#endif /* ARA_LOG_MESSAGE_WRITER_H_ */