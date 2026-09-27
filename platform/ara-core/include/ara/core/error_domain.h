#ifndef ARA_CORE_ERROR_DOMAIN_H_
#define ARA_CORE_ERROR_DOMAIN_H_

#include "ara/core/config.h"
#include "ara/core/core_fwd.h"

#include <cstdint>
#include <exception>
#include <string_view>

namespace ara::core
{
  /* Implements: AP-R3-CORE-001, AP-R3-CORE-002*/
  class ErrorDomain final
  {
    public:
      using IdType = std::uint64_t;

      using ExceptionConverter = void (*)(const ErrorCode&);

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      constexpr ErrorDomain(IdType id, std::string_view name, ExceptionConverter converter) noexcept
          : id_{id}, name_{name}, converter_{converter}
      {
        if (name_.empty() || converter_ == nullptr)
        {
          std::terminate();
        }
      }
#else  // ADAPTIVE_PI_EXCEPTIONS_ENABLED
      constexpr ErrorDomain(IdType id, std::string_view name) noexcept : id_{id}, name_{name}
      {
        if (name_.empty())
        {
          std::terminate();
        }
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED
      [[nodiscard]] constexpr IdType Id() const noexcept
      {
        return id_;
      }

      [[nodiscard]] constexpr std::string_view Name() const noexcept
      {
        return name_;
      }

      [[nodiscard]] constexpr bool operator==(const ErrorDomain& other) const noexcept
      {
        return id_ == other.id_;
      }

      [[nodiscard]] constexpr bool operator!=(const ErrorDomain& other) const noexcept
      {
        return !(*this == other);
      }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      [[noreturn]] void ThrowAsException(const ErrorCode& error) const
      {
        if (converter_ != nullptr)
        {
          converter_(error);
        }

        /* Missing conversion or a converter that unexpectedly returned. */
        std::terminate();
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

    private:
      IdType id_;
      std::string_view name_;
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      ExceptionConverter converter_;
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED
  };

} // namespace ara::core

#endif /* ARA_CORE_ERROR_DOMAIN_H_ */