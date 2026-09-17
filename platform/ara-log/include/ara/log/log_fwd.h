#ifndef ARA_LOG_LOG_FWD_H_
#define ARA_LOG_LOG_FWD_H_

#include <cstdint>

namespace ara::log
{

  enum class LogLevel : std::uint8_t;
  class Logger;
  class LogStream;

} // namespace ara::log

#endif // ARA_LOG_LOG_FWD_H_