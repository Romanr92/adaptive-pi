#ifndef ARA_CORE_RESULT_H_
#define ARA_CORE_RESULT_H_

#include "ara/core/config.h"
#include "ara/core/core_fwd.h"
#include "ara/core/error_code.h"

#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace ara::core
{
  static constexpr std::size_t c_resultIdx = 0;
  static constexpr std::size_t c_errorIdx = 1;

  namespace detail
  {
    template <typename T, typename E>
    class ResultStorage final
    {

      public:
        static_assert(std::is_nothrow_destructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<E>);

        template <std::size_t Index, typename... Args>
        explicit ResultStorage(std::in_place_index_t<Index> tag, Args&&... args)
        {
          slots_[0].emplace(tag, std::forward<Args>(args)...);
        }

        template <std::size_t Index, typename... Args>
        void emplace(Args&&... args)
        {
          const std::size_t next{1U - active_};

          /* If construction throws, the active slot is untouched */
          slots_[next].emplace(std::in_place_index<Index>, std::forward<Args>(args)...);

          const std::size_t previous{active_};
          active_ = next;
          slots_[previous].reset();
        }

        [[nodiscard]] std::size_t index() const noexcept
        {
          const auto& active_slot = slots_[active_];

          if (!active_slot.has_value())
          {
            // An empty active slot indicates an internal invariant violation.
            std::terminate();
          }

          return active_slot->index();
        }

        template <std::size_t Index>
        decltype(auto) get() & noexcept
        {
          if (index() != Index)
          {
            std::terminate();
          }

          auto& active_slot = slots_[active_];
          if (!active_slot.has_value())
          {
            std::terminate();
          }

          return std::get<Index>(*active_slot);
        }

        template <std::size_t Index>
        decltype(auto) get() const& noexcept
        {
          if (index() != Index)
          {
            std::terminate();
          }

          const auto& active_slot = slots_[active_];
          if (!active_slot.has_value())
          {
            std::terminate();
          }

          return std::get<Index>(*active_slot);
        }

        template <std::size_t Index>
        decltype(auto) get() && noexcept
        {
          if (index() != Index)
          {
            std::terminate();
          }

          auto&& active_slot = std::move(slots_[active_]);
          if (!active_slot.has_value())
          {
            std::terminate();
          }

          return std::get<Index>(std::move(*active_slot));
        }

        /* Implements AP-R3-CORE-010 */
        ResultStorage(const ResultStorage&) = default;
        /* Implements AP-R3-CORE-010 */
        ResultStorage(ResultStorage&&) = default;
        ResultStorage& operator=(const ResultStorage&) = delete;
        ResultStorage& operator=(ResultStorage&&) = delete;

      private:
        std::array<std::optional<std::variant<T, E>>, 2> slots_{};
        std::size_t active_{0};
    };
  } // namespace detail

  /*
   * Implements AP-R3-CORE-006.
   *
   * When T and E are identical, direct construction selects the value
   * alternative. Use FromError(...) to select the error alternative.
   *
   * Emplacement constructs the replacement before destroying the current
   * object. If construction throws, the exception propagates and the
   * current alternative remains alive.
   *
   * This guarantee does not undo side effects performed by the payload
   * constructor, including modifications through aliased arguments.
   *
   * Payload destructors must not throw. Internal storage reserves space
   * for two alternatives to support replacement without a throwing move.
   */

  /* Implements: AP-R3-CORE-005: value or error storage */
  /* Include initial factory/query support for CORE-006/007 */
  template <typename T, typename E>
  class Result final
  {
    public:
      Result(const T& value) : storage_{std::in_place_index<c_resultIdx>, value} {}

      Result(T&& value) : storage_{std::in_place_index<c_resultIdx>, std::move(value)} {}

      template <typename U = E, std::enable_if_t<!std::is_same_v<T, U>, int> = 0>
      explicit Result(const E& error) : storage_{std::in_place_index<c_errorIdx>, error}
      {
      }

      template <typename U = E, std::enable_if_t<!std::is_same_v<T, U>, int> = 0>
      explicit Result(E&& error) : storage_{std::in_place_index<c_errorIdx>, std::move(error)}
      {
      }

      template <typename... Args>
      static Result FromValue(Args&&... args)
      {
        return Result{std::in_place_index<c_resultIdx>, std::forward<Args>(args)...};
      }

      template <typename... Args>
      static Result FromError(Args&&... args)
      {
        return Result{std::in_place_index<c_errorIdx>, std::forward<Args>(args)...};
      }

      template <typename... Args>
      void EmplaceValue(Args&&... args)
      {
        storage_.template emplace<c_resultIdx>(std::forward<Args>(args)...);
      }

      template <typename... Args>
      void EmplaceError(Args&&... args)
      {
        storage_.template emplace<c_errorIdx>(std::forward<Args>(args)...);
      }

      /* Implements AP-R3-CORE-007: HasValue reports whether the result stores a value. */
      [[nodiscard]] bool HasValue() const noexcept
      {
        return storage_.index() == c_resultIdx;
      }

      /* Implements AP-R3-CORE-007: operator bool mirrors HasValue. */
      [[nodiscard]] explicit operator bool() const noexcept
      {
        return HasValue();
      }

      T& Value() & noexcept
      {
        return storage_.template get<c_resultIdx>();
      }

      const T& Value() const& noexcept
      {
        return storage_.template get<c_resultIdx>();
      }

      T&& Value() && noexcept
      {
        return std::move(storage_).template get<c_resultIdx>();
      }

      /* Implements AP-R3-CORE-009 */
      const E& Error() const& noexcept
      {
        if (HasValue())
        {
          std::terminate();
        }

        return storage_.template get<c_errorIdx>();
      }

      /* Implements AP-R3-CORE-009 */
      E&& Error() && noexcept
      {
        if (HasValue())
        {
          std::terminate();
        }

        return std::move(storage_).template get<c_errorIdx>();
      }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      T& ValueOrThrow() &
      {
        ThrowIfError();
        return Value();
      }

      const T& ValueOrThrow() const&
      {
        ThrowIfError();
        return Value();
      }

      T&& ValueOrThrow() &&
      {
        ThrowIfError();
        return std::move(*this).Value();
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

      /* Implements AP-R3-CORE-010 */
      Result(const Result&) = default;
      /* Implements AP-R3-CORE-010 */
      Result(Result&&) = default;

      /* Deferred until we design safe state replacement */
      Result& operator=(const Result&) = delete;
      Result& operator=(Result&&) = delete;
      /* Deferred until we design safe state replacement */

    private:
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      void ThrowIfError() const
      {
        static_assert(std::is_same_v<E, ErrorCode>, "ValueOrThrow requires ErrorCode");

        if (!HasValue())
        {
          const auto& error = storage_.template get<c_errorIdx>();
          error.Domain().ThrowAsException(error);
        }
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

      template <typename... Args>
      Result(std::in_place_index_t<c_resultIdx>, Args&&... args)
          : storage_{std::in_place_index<c_resultIdx>, std::forward<Args>(args)...}
      {
      }

      template <typename... Args>
      Result(std::in_place_index_t<c_errorIdx>, Args&&... args)
          : storage_{std::in_place_index<c_errorIdx>, std::forward<Args>(args)...}
      {
      }

      /* Inside Result<T, E>: */
      detail::ResultStorage<T, E> storage_;
  };

  /*
   * Implements AP-R3-CORE-006.
   *
   * When T and E are identical, direct construction selects the value
   * alternative. Use FromError(...) to select the error alternative.
   *
   * Emplacement constructs the replacement before destroying the current
   * object. If construction throws, the exception propagates and the
   * current alternative remains alive.
   *
   * This guarantee does not undo side effects performed by the payload
   * constructor, including modifications through aliased arguments.
   *
   * Payload destructors must not throw. Internal storage reserves space
   * for two alternatives to support replacement without a throwing move.
   */

  /* AP-R3-CORE-005: success without a value, or an error */
  template <typename E>
  class Result<void, E> final
  {
    public:
      explicit Result(const E& error) : storage_{std::in_place_index<c_errorIdx>, error} {}

      explicit Result(E&& error) : storage_{std::in_place_index<c_errorIdx>, std::move(error)} {}

      static Result FromValue()
      {
        return Result{};
      }

      template <typename... Args>
      static Result FromError(Args&&... args)
      {
        return Result{std::in_place_index<c_errorIdx>, std::forward<Args>(args)...};
      }

      void EmplaceValue()
      {
        storage_.template emplace<c_resultIdx>();
      }

      template <typename... Args>
      void EmplaceError(Args&&... args)
      {
        storage_.template emplace<c_errorIdx>(std::forward<Args>(args)...);
      }

      /* Implements AP-R3-CORE-007: HasValue reports whether the void result is successful. */
      [[nodiscard]] bool HasValue() const noexcept
      {
        return storage_.index() == c_resultIdx;
      }

      [[nodiscard]] explicit operator bool() const noexcept
      {
        return HasValue();
      }

      /* Implements AP-R3-CORE-008: Void Value checks success and terminates when the result holds an error. */
      void Value() const noexcept
      {
        if (!HasValue())
        {
          std::terminate();
        }
      }

      /* Implements AP-R3-CORE-009 */
      const E& Error() const& noexcept
      {
        if (HasValue())
        {
          std::terminate();
        }

        return storage_.template get<c_errorIdx>();
      }

      /* Implements AP-R3-CORE-009 */
      E&& Error() && noexcept
      {
        if (HasValue())
        {
          std::terminate();
        }

        return std::move(storage_).template get<c_errorIdx>();
      }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      /* Implements AP-R3-CORE-008: ValueOrThrow returns the stored value or converts the ErrorCode through the domain.
       */
      void ValueOrThrow() const
      {
        ThrowIfError();
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

      /* Implements AP-R3-CORE-010 */
      Result(const Result&) = default;
      /* Implements AP-R3-CORE-010 */
      Result(Result&& other) noexcept : storage_{std::move(other.storage_)} {}

      Result& operator=(const Result&) = delete;
      Result& operator=(Result&&) = delete;

    private:
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      void ThrowIfError() const
      {
        static_assert(std::is_same_v<E, ErrorCode>, "ValueOrThrow requires ErrorCode");

        if (!HasValue())
        {
          const auto& error = storage_.template get<c_errorIdx>();
          error.Domain().ThrowAsException(error);
        }
      }
#endif // ADAPTIVE_PI_EXCEPTIONS_ENABLED

      Result() : storage_{std::in_place_index<c_resultIdx>} {}

      template <typename... Args>
      Result(std::in_place_index_t<c_errorIdx>, Args&&... args)
          : storage_{std::in_place_index<c_errorIdx>, std::forward<Args>(args)...}
      {
      }

      // Inside Result<void, E>:
      detail::ResultStorage<std::monostate, E> storage_;
  };

} // namespace ara::core

#endif // ARA_CORE_RESULT_H_