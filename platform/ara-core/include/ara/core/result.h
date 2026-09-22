#ifndef ARA_CORE_RESULT_H_
#define ARA_CORE_RESULT_H_

#include "ara/core/core_fwd.h"
#include "ara/core/error_code.h"

#include <utility>
#include <variant>

namespace ara::core
{
  static constexpr std::size_t c_resultIdx = 0;
  static constexpr std::size_t c_errorIdx = 1;

  /* Implements: AP-R3-CORE-005: value or error storage */
  /* Include initial factory/query support for CORE-006/007 */
  template <typename T, typename E>
  class Result final
  {
    public:
      static Result FromValue(T value)
      {
        return Result{std::in_place_index<c_resultIdx>, std::move(value)};
      }

      static Result FromError(E error)
      {
        return Result{std::in_place_index<c_errorIdx>, std::move(error)};
      }

      [[nodiscard]] bool HasValue() const noexcept
      {
        return storage_.index() == 0;
      }

      Result(const Result&) = default;
      Result(Result&&) = default;

      /* Deferred until we design safe state replacement */
      Result& operator=(const Result&) = delete;
      Result& operator=(Result&&) = delete;
      /* Deferred until we design safe state replacement */

    private:
      Result(std::in_place_index_t<c_resultIdx> tag, T value) : storage_{tag, std::move(value)} {}
      Result(std::in_place_index_t<c_errorIdx> tag, E error) : storage_{tag, std::move(error)} {}

      std::variant<T, E> storage_;
  };

  /* AP-R3-CORE-005: success without a value, or an error */
  template <typename E>
  class Result<void, E> final
  {
    public:
      static Result FromValue()
      {
        return Result{};
      }

      static Result FromError(E error)
      {
        return Result{std::in_place_index<c_errorIdx>, std::move(error)};
      }

      [[nodiscard]] bool HasValue() const noexcept
      {
        return storage_.index() == 0;
      }

      Result(const Result&) = default;
      Result(Result&&) = default;

      Result& operator=(const Result&) = delete;
      Result& operator=(Result&&) = delete;

    private:
      Result() : storage_{std::in_place_index<c_resultIdx>} {}
      Result(std::in_place_index_t<c_errorIdx> tag, E error) : storage_{tag, std::move(error)} {}

      std::variant<std::monostate, E> storage_;
  };

} // namespace ara::core
#endif // ARA_CORE_RESULT_H_
