#ifndef ARA_LOG_MESSAGE_ATTRIBUTES_H_
#define ARA_LOG_MESSAGE_ATTRIBUTES_H_

#include <cstdint>
#include <string_view>

namespace ara::log
{
  /* Implements AP-R3-LOG-010 */
  struct Location
  {
      std::string_view file;
      std::uint32_t line;
  };

  /* Implements AP-R3-LOG-010 */
  struct Tag
  {
      std::string_view value;
  };
} // namespace ara::log

#endif /* ARA_LOG_MESSAGE_ATTRIBUTES_H_ */