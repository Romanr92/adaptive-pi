#include "record_buffer.h"

#include <cstring>

namespace ara::log::detail
{
  /* Supports AP-R3-LOG-005, AP-R3-LOG-006, AP-R3-LOG-008 */
  bool RecordBuffer::Append(std::string_view value) noexcept
  {
    if (value.size() > buffer_.size() - size_)
    {
      return false;
    }

    if (!value.empty())
    {
      std::memcpy(buffer_.data() + size_, value.data(), value.size());
      size_ += value.size();
    }

    return true;
  }

  /* Supports AP-R3-LOG-006 */
  std::string_view RecordBuffer::View() const noexcept
  {
    return {buffer_.data(), size_};
  }

} // namespace ara::log::detail
