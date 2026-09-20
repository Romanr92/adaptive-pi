#ifndef ARA_CORE_ERROR_DOMAIN_H_
#define ARA_CORE_ERROR_DOMAIN_H_

#include <cstdint>
#include <string_view>

namespace ara::core
{

  class ErrorDomain final
  {
    public:
      using IdType = std::uint64_t;

      constexpr ErrorDomain(IdType id, std::string_view name) noexcept : id_{id}, name_{name} {}

      [[nodiscard]] constexpr IdType Id() const noexcept
      {
        return id_;
      }

      [[nodiscard]] constexpr std::string_view Name() const noexcept
      {
        return name_;
      }

    private:
      IdType id_;
      std::string_view name_;
  };

} // namespace ara::core

#endif /* ARA_CORE_ERROR_DOMAIN_H_ */