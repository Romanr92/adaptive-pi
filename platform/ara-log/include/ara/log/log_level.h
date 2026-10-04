#ifndef ARA_LOG_LOG_LEVEL_H_
#define ARA_LOG_LOG_LEVEL_H_

#include <cstdint>

namespace ara::log
{

  // AP-R3-LOG-001
  enum class LogLevel : std::uint8_t
  {
    kOff = 0x00,
    kFatal = 0x01,
    kError = 0x02,
    kWarn = 0x03,
    kInfo = 0x04,
    kDebug = 0x05,
    kVerbose = 0x06
  };
} // namespace ara::log

#endif /* ARA_LOG_LOG_LEVEL_H_ */