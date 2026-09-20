#ifndef ARA_CORE_ADAPTIVE_PI_ERROR_DOMAIN_H_
#define ARA_CORE_ADAPTIVE_PI_ERROR_DOMAIN_H_

#include "ara/core/error_code.h"

namespace ara::core
{
  /* Implements AP-R3-CORE-004 */
  /* Domain error enums deliberately use ErrorCode::ValueType so their
     representation matches ErrorCode across every supported target. Future domain values are not constrained to the
     current three small numbers. NOLINTNEXTLINE(performance-enum-size) */
  enum class AdaptivePiErrc : ErrorCode::ValueType
  {
    kInvalidArgument = 1,
    kInvalidState = 2,
    kOperationFailed = 3
  };

  /* The high bytes encode ASCII "API" for AdaptivePi, while the low-order value
     1 identifies the first project-owned domain. This published ID remains stable. */
  inline constexpr ErrorDomain kAdaptivePiErrorDomain{0x4150490000000001ULL, "AdaptivePi"};

  [[nodiscard]] constexpr const ErrorDomain& GetAdaptivePiErrorDomain() noexcept
  {
    return kAdaptivePiErrorDomain;
  }

  [[nodiscard]] constexpr ErrorCode MakeErrorCode(AdaptivePiErrc error) noexcept
  {
    return ErrorCode{static_cast<ErrorCode::ValueType>(error), GetAdaptivePiErrorDomain()};
  }

  [[nodiscard]] constexpr bool operator==(const ErrorCode& error_code, AdaptivePiErrc error) noexcept
  {
    return error_code == MakeErrorCode(error);
  }

  [[nodiscard]] constexpr bool operator==(AdaptivePiErrc error, const ErrorCode& error_code) noexcept
  {
    return error_code == error;
  }

  [[nodiscard]] constexpr bool operator!=(const ErrorCode& error_code, AdaptivePiErrc error) noexcept
  {
    return !(error_code == error);
  }

  [[nodiscard]] constexpr bool operator!=(AdaptivePiErrc error, const ErrorCode& error_code) noexcept
  {
    return !(error == error_code);
  }

} // namespace ara::core

#endif /* ARA_CORE_ADAPTIVE_PI_ERROR_DOMAIN_H_ */