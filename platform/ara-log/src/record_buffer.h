#ifndef ADAPTIVE_PI_ARA_LOG_RECORD_BUFFER_H_
#define ADAPTIVE_PI_ARA_LOG_RECORD_BUFFER_H_

#include <array>
#include <cstddef>
#include <string_view>

namespace ara::log::detail
{

  /* Supports AP-R3-LOG-005, AP-R3-LOG-006, AP-R3-LOG-008 */
  class RecordBuffer final
  {
    public:
      [[nodiscard]] bool Append(std::string_view value) noexcept;
      [[nodiscard]] std::string_view View() const noexcept;

    private:
      // Fixed capacity; checked appends reject oversized records.
      static constexpr std::size_t c_record_capacity = 16384;
      static constexpr std::size_t c_record_zero = 0;

      std::array<char, c_record_capacity> buffer_{};
      std::size_t size_ = c_record_zero;
  };
} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_RECORD_BUFFER_H_ */