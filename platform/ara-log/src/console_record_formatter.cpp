#include "console_record_formatter.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <limits>
#include <system_error>

namespace ara::log::detail
{
  namespace
  {
    /* Implements AP-R3-LOG-006 escaping */
    [[nodiscard]] bool AppendEscaped(RecordBuffer& output, std::string_view value, bool escape_brackets) noexcept
    {
      for (const char character : value)
      {
        std::string_view replacement;

        switch (character)
        {
        case '\\':
          replacement = "\\\\";
          break;
        case '\n':
          replacement = "\\n";
          break;
        case '\r':
          replacement = "\\r";
          break;
        case '[':
          replacement = escape_brackets ? "\\[" : "[";
          break;
        case ']':
          replacement = escape_brackets ? "\\]" : "]";
          break;

        default:
          replacement = std::string_view{&character, 1};
          break;
        }

        if (!output.Append(replacement))
        {
          return false;
        }
      }

      return true;
    }

    /* Implements AP-R3-LOG-006 severity rendering */
    [[nodiscard]] std::string_view LevelName(LogLevel level) noexcept
    {
      switch (level)
      {
      case LogLevel::kFatal:
        return "FATAL";
      case LogLevel::kError:
        return "ERROR";
      case LogLevel::kWarn:
        return "WARN";
      case LogLevel::kInfo:
        return "INFO";
      case LogLevel::kDebug:
        return "DEBUG";
      case LogLevel::kVerbose:
        return "VERBOSE";

      default:
        return {};
      }
    }

    /* Implements AP-R3-LOG-006 decimal ID rendering */
    [[nodiscard]] bool AppendDecimal(RecordBuffer& output, std::int64_t value) noexcept
    {
      // Space for all decimal digits and possible minus sign.
      constexpr std::size_t c_decimal_capacity = std::numeric_limits<std::int64_t>::digits10 + 2;

      std::array<char, c_decimal_capacity> text{};
      const auto result = std::to_chars(text.data(), text.data() + text.size(), value);

      if (result.ec != std::errc{})
      {
        return false;
      }

      const auto length = static_cast<std::size_t>(result.ptr - text.data());

      return output.Append(std::string_view{text.data(), length});
    }

    /* Implements AP-R3-LOG-006 UTC timestamp rendering */
    [[nodiscard]] bool AppendTimestamp(RecordBuffer& output, std::int64_t unix_miliseconds) noexcept
    {
      constexpr std::int64_t c_milis_per_second = 1000;

      // std::tm stores years since 1900 and months starting at zero.
      constexpr int c_tm_year_base = 1900;
      constexpr int c_tm_month_adjustment = 1;

      // The console format requires a four-digit year.
      constexpr int c_minimum_year = 0;
      constexpr int c_maximum_year = 9999;

      // Normalize negative timestamps so the remainder is always 0-999.
      auto seconds = unix_miliseconds / c_milis_per_second;
      auto miliseconds = unix_miliseconds % c_milis_per_second;

      // 24 timestamp characters plus the snprintf null terminator.
      constexpr std::size_t c_timestamp_length = 24;
      constexpr std::size_t c_null_terminator_size = 1;

      // UTC timestamp: YYYY-MM-DDTHH:MM:SS.mmmZ.
      constexpr char c_timestamp_format[] = "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";

      if (miliseconds < 0)
      {
        --seconds;
        miliseconds += c_milis_per_second;
      }

      // Supported Linux host and ARM64 targets use signed 64-bit time_t.
      static_assert(std::numeric_limits<std::time_t>::is_signed);
      static_assert(std::numeric_limits<std::time_t>::digits >= std::numeric_limits<std::int64_t>::digits);

      const auto timestamp = static_cast<std::time_t>(seconds);
      std::tm utc{};

      if (::gmtime_r(&timestamp, &utc) == nullptr)
      {
        return false;
      }

      // The required format reserves exactly four digits for the year.
      if ((utc.tm_year < (c_minimum_year - c_tm_year_base)) || (utc.tm_year > (c_maximum_year - c_tm_year_base)))
      {
        return false;
      }

      std::array<char, c_timestamp_length + c_null_terminator_size> text{};

      const int written = std::snprintf(text.data(), text.size(), c_timestamp_format, utc.tm_year + c_tm_year_base,
                                        utc.tm_mon + c_tm_month_adjustment, utc.tm_mday, utc.tm_hour, utc.tm_min,
                                        utc.tm_sec, static_cast<int>(miliseconds));

      if (written != static_cast<int>(c_timestamp_length))
      {
        return false;
      }

      return output.Append(std::string_view{text.data(), c_timestamp_length});
    }

  } // namespace

  bool RenderConsoleRecord(const RuntimeMetadata& metadata, LogLevel level, std::string_view context,
                           std::string_view message, RecordBuffer& output) noexcept
  {
    const auto level_name = LevelName(level);

    if ((!output.View().empty()) || (level_name.empty()) || (metadata.process_id <= 0) || (metadata.thread_id <= 0))
    {
      return false;
    }

    // Short-circuit evaluation stops at the first formatting failure.
    return output.Append("[") && AppendTimestamp(output, metadata.unix_milliseconds) && output.Append("][") &&
           AppendDecimal(output, metadata.process_id) && output.Append("][") &&
           AppendDecimal(output, metadata.thread_id) && output.Append("][") && output.Append(level_name) &&
           output.Append("][") && AppendEscaped(output, context, true) && output.Append("] ") &&
           AppendEscaped(output, message, false) && output.Append("\n");
  }

} // namespace ara::log::detail
