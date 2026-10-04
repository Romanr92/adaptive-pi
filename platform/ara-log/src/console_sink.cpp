#include "console_sink.h"

#include <cstdio>

namespace ara::log::detail
{

  /* Implements AP-R3-LOG-004 */
  SinkStatus ConsoleSink::Write(std::string_view record) noexcept
  {
    if (record.empty())
    {
      return SinkStatus::kSuccess;
    }

    const auto written = std::fwrite(record.data(), sizeof(char), record.size(), stdout);

    if (written != record.size())
    {
      return SinkStatus::kWriteFailed;
    }

    return SinkStatus::kSuccess;
  }

} // namespace ara::log::detail
