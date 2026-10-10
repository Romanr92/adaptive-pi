#ifndef ARA_LOG_MODELLED_MESSAGE_H_
#define ARA_LOG_MODELLED_MESSAGE_H_

#include "ara/log/message_attributes.h"

#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace ara::log::detail
{
  /* Supports AP-R3-LOG-010 */
  template <typename T>
  using NormalizedType = std::remove_cv_t<std::remove_reference_t<T>>;

  template <typename Tuple>
  struct NormalizeParameterTuple;

  template <typename... Types>
  struct NormalizeParameterTuple<std::tuple<Types...>>
  {
      using Type = std::tuple<NormalizedType<Types>...>;
  };

  /* Supports AP-R3-LOG-010 */
  template <typename DeclaredTuple, typename... Params>
  inline constexpr bool kParameterMatch =
    std::is_same_v<typename NormalizeParameterTuple<DeclaredTuple>::Type, std::tuple<NormalizedType<Params>...>>;

  /* Supports Ap-R3-LOG-010 */
  constexpr bool IsValidMessageId(std::string_view id) noexcept
  {
    if (id.empty())
    {
      return false;
    }

    for (const char character : id)
    {
      const bool allowed = (((character >= 'A') && (character <= 'Z')) || ((character >= 'a') && (character <= 'z')) ||
                            ((character >= '0') && (character <= '9')) || (character == '_'));

      if (!allowed)
      {
        return false;
      }
    }

    return true;
  }

  /* Supports AP-R3-LOG-010 */
  template <typename T>
  inline constexpr bool kSupportedAttribute =
    std::is_same_v<NormalizedType<T>, Location> || std::is_same_v<NormalizedType<T>, Tag>;

  /* Supports AP-R3-LOG-010 */
  template <typename... Attrs>
  inline constexpr bool kValidAttributes =
    ((kSupportedAttribute<Attrs> && ...) &&
     ((std::size_t{0} + ... + (std::is_same_v<NormalizedType<Attrs>, Location> ? std::size_t{1} : std::size_t{0})) <=
      std::size_t{1}) &&
     ((std::size_t{0} + ... + (std::is_same_v<NormalizedType<Attrs>, Tag> ? std::size_t{1} : std::size_t{0})) <=
      std::size_t{1}));

} // namespace ara::log::detail

#endif /* ARA_LOG_MODELLED_MESSAGE_H_ */