#ifndef ARA_CORE_ERROR_CODE_H_
#define ARA_CORE_ERROR_CODE_H_

#include "ara/core/error_domain.h"

#include <cstdint>

namespace ara::core
{
  /* Implements AP-R3-CORE-003, AP-R3-CORE-004 */
  class ErrorCode final
  {
    public:
      using ValueType = std::int32_t;

      constexpr ErrorCode(ValueType value, const ErrorDomain& domain) noexcept : value_{value}, domain_{&domain} {}

      [[nodiscard]] constexpr ValueType Value() const noexcept
      {
        return value_;
      }

      [[nodiscard]] constexpr const ErrorDomain& Domain() const noexcept
      {
        return *domain_;
      }

      [[nodiscard]] constexpr bool operator==(const ErrorCode& other) const noexcept
      {
        return ((value_ == other.value_) && (Domain() == other.Domain()));
      }

      [[nodiscard]] constexpr bool operator!=(const ErrorCode& other) const noexcept
      {
        return !(*this == other);
      }

    private:
      ValueType value_;
      const ErrorDomain* domain_;
  };

} // namespace ara::core

#endif /* ARA_CORE_ERROR_CODE_H_ */