#ifndef ADAPTIVE_PI_ARA_LOG_CONSOLE_RECORD_FORMATTER_H_
#define ADAPTIVE_PI_ARA_LOG_CONSOLE_RECORD_FORMATTER_H_

#include "ara/log/log_level.h"
#include "metadata_provider.h"
#include "record_buffer.h"

#include <string_view>

namespace ara::log::detail
{
  /* Implements AP-R3-LOG-006; supports AP-R3-LOG-008 */
  [[nodiscard]] bool RenderConsoleRecord(const RuntimeMetadata& metadata, LogLevel level, std::string_view context,
                                         std::string_view message, RecordBuffer& output) noexcept;
} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_CONSOLE_RECORD_FORMATTER_H_ */