#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/error_domain.h"
#include "ara/core/result.h"

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ara::core
{
  namespace
  {
    /* Narrow test-only access to the members used by the fault-injection probes.
     * Explicit instantiation permits naming a private member as a template
     * argument ([temp.explicit] in C++17). The injected friend belongs to this
     * test-only tag, not to a production class. All production headers are parsed
     * unchanged, so their definitions remain identical across translation units.
     * The concrete member allowlist is at the end of this translation unit.
     */
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  /* Intentionally declare one non-template friend overload per concrete tag. */
  #pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
    template <typename Owner, typename Member>
    struct PrivateMemberTag
    {
        using Pointer = Member Owner::*;
        friend Pointer TestMemberPointer(PrivateMemberTag) noexcept;
    };
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif

    template <typename Tag, typename Tag::Pointer Member>
    struct ExposeTestMember
    {
        friend typename Tag::Pointer TestMemberPointer(Tag) noexcept
        {
          return Member;
        }
    };

    template <typename T, typename E>
    using TestStorage = detail::ResultStorage<std::conditional_t<std::is_void_v<T>, std::monostate, T>, E>;

    template <typename T, typename E>
    using TestSlots =
      std::array<std::optional<std::variant<std::conditional_t<std::is_void_v<T>, std::monostate, T>, E>>, 2>;

    /* Register the three storage members together, only for the explicitly
     * listed Result specializations at the end of this test translation unit.
     */
    template <typename T, typename E, auto StorageMember, auto SlotsMember, auto ActiveMember>
    struct ExposeResultProbe : ExposeTestMember<PrivateMemberTag<Result<T, E>, TestStorage<T, E>>, StorageMember>,
                               ExposeTestMember<PrivateMemberTag<TestStorage<T, E>, TestSlots<T, E>>, SlotsMember>,
                               ExposeTestMember<PrivateMemberTag<TestStorage<T, E>, std::size_t>, ActiveMember>
    {
    };

    template <typename T, typename E>
    decltype(auto) StorageOf(Result<T, E>& result) noexcept
    {
      return (result.*TestMemberPointer(PrivateMemberTag<Result<T, E>, TestStorage<T, E>>{}));
    }

    template <typename T, typename E>
    decltype(auto) StorageOf(const Result<T, E>& result) noexcept
    {
      return (result.*TestMemberPointer(PrivateMemberTag<Result<T, E>, TestStorage<T, E>>{}));
    }

    template <typename T, typename E>
    decltype(auto) SlotsOf(detail::ResultStorage<T, E>& storage) noexcept
    {
      using Slots = std::array<std::optional<std::variant<T, E>>, 2>;
      return (storage.*TestMemberPointer(PrivateMemberTag<detail::ResultStorage<T, E>, Slots>{}));
    }

    template <typename T, typename E>
    std::size_t& ActiveIndexOf(detail::ResultStorage<T, E>& storage) noexcept
    {
      return storage.*TestMemberPointer(PrivateMemberTag<detail::ResultStorage<T, E>, std::size_t>{});
    }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    template struct ExposeTestMember<PrivateMemberTag<ErrorDomain, ErrorDomain::ExceptionConverter>,
                                     &ErrorDomain::converter_>;

    ErrorDomain::ExceptionConverter& ExceptionConverterOf(ErrorDomain& domain) noexcept
    {
      return domain.*TestMemberPointer(PrivateMemberTag<ErrorDomain, ErrorDomain::ExceptionConverter>{});
    }
#endif

    /* ========================== Test_AP_R3_CORE_005 ==================================== */

    /* ----------------------------------------------------------------------------------- */

    struct ResultStateCase_ReportsSelectedState
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultStateCase_ReportsSelectedState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that the selected result state is reported correctly.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultHasExactlyOneState
        : public ::testing::TestWithParam<ResultStateCase_ReportsSelectedState>
    {
    };

    /* Verify that the selected result state is reported correctly.
     * 1. Arrange: Read the case parameters and prepare the selected result state.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, ReportsSelectedState)
    {
      /* Arrange */
      const ResultStateCase_ReportsSelectedState& parameter{GetParam()};

      /* Act */
      const auto result = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                              : Result<int>::FromError(MakeErrorCode(parameter.error));

      /* Expect */
      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    std::string ResultStateCaseName_ReportsSelectedState(
      const ::testing::TestParamInfo<ResultStateCase_ReportsSelectedState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState,
      ::testing::Values(ResultStateCase_ReportsSelectedState{true, 0, AdaptivePiErrc::kInvalidArgument, "ZeroValue",
                                                             "Zero is a successful value"},
                        ResultStateCase_ReportsSelectedState{true, 42, AdaptivePiErrc::kInvalidState, "PositiveValue",
                                                             "Positive integer success"},
                        ResultStateCase_ReportsSelectedState{true, -1, AdaptivePiErrc::kOperationFailed,
                                                             "NegativeValue", "Negative integer success"},
                        ResultStateCase_ReportsSelectedState{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                             "InvalidArgument", "Invalid argument failure"},
                        ResultStateCase_ReportsSelectedState{false, 42, AdaptivePiErrc::kInvalidState, "InvalidState",
                                                             "Invalid state failure"},
                        ResultStateCase_ReportsSelectedState{false, -1, AdaptivePiErrc::kOperationFailed,
                                                             "OperationFailed", "Operation failed"}),
      ResultStateCaseName_ReportsSelectedState);

    /* ----------------------------------------------------------------------------------- */

    struct ResultStateCase_CopyConstructionPreservesState
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultStateCase_CopyConstructionPreservesState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that copying preserves both source and destination states.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultHasExactlyOneState_CopyConstructionPreservesState
        : public ::testing::TestWithParam<ResultStateCase_CopyConstructionPreservesState>
    {
    };

    /* Verify that copying preserves both source and destination states.
     * 1. Arrange: Read the case parameters and construct the source result.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState_CopyConstructionPreservesState, CopyConstructionPreservesState)
    {
      /* Arrange */
      const ResultStateCase_CopyConstructionPreservesState& parameter{GetParam()};
      const auto source = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                              : Result<int>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      const Result<int> destination{source};

      /* Expect */
      EXPECT_EQ(destination.HasValue(), parameter.has_value);
      EXPECT_EQ(source.HasValue(), parameter.has_value);
    }

    std::string ResultStateCaseName_CopyConstructionPreservesState(
      const ::testing::TestParamInfo<ResultStateCase_CopyConstructionPreservesState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState_CopyConstructionPreservesState,
      ::testing::Values(ResultStateCase_CopyConstructionPreservesState{true, 0, AdaptivePiErrc::kInvalidArgument,
                                                                       "ZeroValue", "Zero is a successful value"},
                        ResultStateCase_CopyConstructionPreservesState{true, 42, AdaptivePiErrc::kInvalidState,
                                                                       "PositiveValue", "Positive integer success"},
                        ResultStateCase_CopyConstructionPreservesState{true, -1, AdaptivePiErrc::kOperationFailed,
                                                                       "NegativeValue", "Negative integer success"},
                        ResultStateCase_CopyConstructionPreservesState{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                                       "InvalidArgument", "Invalid argument failure"},
                        ResultStateCase_CopyConstructionPreservesState{false, 42, AdaptivePiErrc::kInvalidState,
                                                                       "InvalidState", "Invalid state failure"},
                        ResultStateCase_CopyConstructionPreservesState{false, -1, AdaptivePiErrc::kOperationFailed,
                                                                       "OperationFailed", "Operation failed"}),
      ResultStateCaseName_CopyConstructionPreservesState);

    /* ----------------------------------------------------------------------------------- */

    /* Also exercises AP-R3-CORE-010. Do not observe the moved-from result. */
    struct ResultStateCase_MoveConstructionPreservesState
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultStateCase_MoveConstructionPreservesState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that moving preserves the destination state without inspecting the
     * moved-from object. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultHasExactlyOneState_MoveConstructionPreservesState
        : public ::testing::TestWithParam<ResultStateCase_MoveConstructionPreservesState>
    {
    };

    /* Verify that moving preserves the destination state without inspecting the moved-from object.
     * 1. Arrange: Read the case parameters and construct the source result.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState_MoveConstructionPreservesState, MoveConstructionPreservesState)
    {
      /* Arrange */
      const ResultStateCase_MoveConstructionPreservesState& parameter{GetParam()};
      auto source = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                        : Result<int>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      /* Explicitly exercise move construction even when the payload is trivial. */
      /* NOLINTNEXTLINE(performance-move-const-arg) */
      const Result<int> destination{std::move(source)};

      /* Expect */
      EXPECT_EQ(destination.HasValue(), parameter.has_value);
    }

    std::string ResultStateCaseName_MoveConstructionPreservesState(
      const ::testing::TestParamInfo<ResultStateCase_MoveConstructionPreservesState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState_MoveConstructionPreservesState,
      ::testing::Values(ResultStateCase_MoveConstructionPreservesState{true, 0, AdaptivePiErrc::kInvalidArgument,
                                                                       "ZeroValue", "Zero is a successful value"},
                        ResultStateCase_MoveConstructionPreservesState{true, 42, AdaptivePiErrc::kInvalidState,
                                                                       "PositiveValue", "Positive integer success"},
                        ResultStateCase_MoveConstructionPreservesState{true, -1, AdaptivePiErrc::kOperationFailed,
                                                                       "NegativeValue", "Negative integer success"},
                        ResultStateCase_MoveConstructionPreservesState{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                                       "InvalidArgument", "Invalid argument failure"},
                        ResultStateCase_MoveConstructionPreservesState{false, 42, AdaptivePiErrc::kInvalidState,
                                                                       "InvalidState", "Invalid state failure"},
                        ResultStateCase_MoveConstructionPreservesState{false, -1, AdaptivePiErrc::kOperationFailed,
                                                                       "OperationFailed", "Operation failed"}),
      ResultStateCaseName_MoveConstructionPreservesState);

    /* ----------------------------------------------------------------------------------- */

    struct ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that identical value and error types still have distinct states.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultHasExactlyOneState_SameTypesAndPayloadsKeepAlternativesDistinct
        : public ::testing::TestWithParam<ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct>
    {
    };

    /* Verify that identical value and error types still have distinct states.
     * 1. Arrange: Read the selected alternative and the integer used for either payload.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState_SameTypesAndPayloadsKeepAlternativesDistinct,
           SameTypesAndPayloadsKeepAlternativesDistinct)
    {
      /* Arrange */
      const ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct& parameter{GetParam()};

      /* Act */
      const auto result = parameter.has_value ? Result<int, int>::FromValue(parameter.value)
                                              : Result<int, int>::FromError(parameter.value);

      /* Expect */
      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    std::string ResultStateCaseName_SameTypesAndPayloadsKeepAlternativesDistinct(
      const ::testing::TestParamInfo<ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState_SameTypesAndPayloadsKeepAlternativesDistinct,
      ::testing::Values(
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{true, 0, AdaptivePiErrc::kInvalidArgument,
                                                                     "ZeroValue", "Zero is a successful value"},
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{true, 42, AdaptivePiErrc::kInvalidState,
                                                                     "PositiveValue", "Positive integer success"},
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{true, -1, AdaptivePiErrc::kOperationFailed,
                                                                     "NegativeValue", "Negative integer success"},
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                                     "InvalidArgument", "Invalid argument failure"},
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{false, 42, AdaptivePiErrc::kInvalidState,
                                                                     "InvalidState", "Invalid state failure"},
        ResultStateCase_SameTypesAndPayloadsKeepAlternativesDistinct{false, -1, AdaptivePiErrc::kOperationFailed,
                                                                     "OperationFailed", "Operation failed"}),
      ResultStateCaseName_SameTypesAndPayloadsKeepAlternativesDistinct);

    /* ----------------------------------------------------------------------------------- */

    struct ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative& parameter)
    {
      return output << parameter.description;
    }

    class TrackedObject_ConstructsAndDestroysOnlySelectedAlternative final
    {
      public:
        explicit TrackedObject_ConstructsAndDestroysOnlySelectedAlternative(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        TrackedObject_ConstructsAndDestroysOnlySelectedAlternative(
          const TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&) = delete;
        TrackedObject_ConstructsAndDestroysOnlySelectedAlternative(
          TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&& other) noexcept
            : TrackedObject_ConstructsAndDestroysOnlySelectedAlternative{other.live_count_}
        {
        }
        TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&
        operator=(const TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&) = delete;
        TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&
        operator=(TrackedObject_ConstructsAndDestroysOnlySelectedAlternative&&) = delete;

        ~TrackedObject_ConstructsAndDestroysOnlySelectedAlternative()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    /* Provides the case data for this check: Verify construction and destruction of only the selected alternatives.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultHasExactlyOneState_ConstructsAndDestroysOnlySelectedAlternative
        : public ::testing::TestWithParam<ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative>
    {
    };

    /* Verify construction and destruction of only the selected alternatives.
     * 1. Arrange: Initialize the lifetime counters and storage for observations.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct scoped results and record their state and live payload counts before destruction.
     * 3. Expect: Only selected payloads were alive and all live counts return to zero after scope exit.
     */
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState_ConstructsAndDestroysOnlySelectedAlternative,
           ConstructsAndDestroysOnlySelectedAlternative)
    {
      /* Arrange */
      int guard_live{0};
      auto guard_probe = Result<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
                                TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::
        FromValue(TrackedObject_ConstructsAndDestroysOnlySelectedAlternative{guard_live});

      const ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative& parameter{GetParam()};
      int live_values{0};
      int live_errors{0};
      bool selected_has_value{};
      int values_while_alive{};
      int errors_while_alive{};

      /* Act */
      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      {
        const auto result = parameter.has_value
                              ? Result<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
                                       TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::
                                  FromValue(TrackedObject_ConstructsAndDestroysOnlySelectedAlternative{live_values})
                              : Result<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
                                       TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::
                                  FromError(TrackedObject_ConstructsAndDestroysOnlySelectedAlternative{live_errors});

        selected_has_value = result.HasValue();
        values_while_alive = live_values;
        errors_while_alive = live_errors;
      }

      /* Expect */
      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_EQ(selected_has_value, parameter.has_value);
      EXPECT_EQ(values_while_alive, parameter.has_value ? 1 : 0);
      EXPECT_EQ(errors_while_alive, parameter.has_value ? 0 : 1);

      EXPECT_EQ(live_values, 0);
      EXPECT_EQ(live_errors, 0);
    }

    std::string ResultStateCaseName_ConstructsAndDestroysOnlySelectedAlternative(
      const ::testing::TestParamInfo<ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState_ConstructsAndDestroysOnlySelectedAlternative,
      ::testing::Values(
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{true, 0, AdaptivePiErrc::kInvalidArgument,
                                                                     "ZeroValue", "Zero is a successful value"},
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{true, 42, AdaptivePiErrc::kInvalidState,
                                                                     "PositiveValue", "Positive integer success"},
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{true, -1, AdaptivePiErrc::kOperationFailed,
                                                                     "NegativeValue", "Negative integer success"},
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                                     "InvalidArgument", "Invalid argument failure"},
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{false, 42, AdaptivePiErrc::kInvalidState,
                                                                     "InvalidState", "Invalid state failure"},
        ResultStateCase_ConstructsAndDestroysOnlySelectedAlternative{false, -1, AdaptivePiErrc::kOperationFailed,
                                                                     "OperationFailed", "Operation failed"}),
      ResultStateCaseName_ConstructsAndDestroysOnlySelectedAlternative);

    /* ----------------------------------------------------------------------------------- */

    struct ResultVoidStateCase_ReportsSelectedState
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidStateCase_ReportsSelectedState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that the selected result state is reported correctly.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultVoidHasExactlyOneState
        : public ::testing::TestWithParam<ResultVoidStateCase_ReportsSelectedState>
    {
    };

    /* Verify that the selected result state is reported correctly.
     * 1. Arrange: Read the case parameters and prepare the selected result state.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState, ReportsSelectedState)
    {
      /* Arrange */
      const ResultVoidStateCase_ReportsSelectedState& parameter{GetParam()};

      /* Act */
      const auto result =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));

      /* Expect */
      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    std::string ResultVoidStateCaseName_ReportsSelectedState(
      const ::testing::TestParamInfo<ResultVoidStateCase_ReportsSelectedState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_005_ResultVoidHasExactlyOneState,
      ::testing::Values(ResultVoidStateCase_ReportsSelectedState{true, AdaptivePiErrc::kInvalidArgument, "Success",
                                                                 "Successful completion without a value"},
                        ResultVoidStateCase_ReportsSelectedState{false, AdaptivePiErrc::kInvalidArgument,
                                                                 "InvalidArgument", "Invalid argument failure"},
                        ResultVoidStateCase_ReportsSelectedState{false, AdaptivePiErrc::kInvalidState, "InvalidState",
                                                                 "Invalid state failure"},
                        ResultVoidStateCase_ReportsSelectedState{false, AdaptivePiErrc::kOperationFailed,
                                                                 "OperationFailed", "Operation failed"}),
      ResultVoidStateCaseName_ReportsSelectedState);

    /* ----------------------------------------------------------------------------------- */

    struct ResultVoidStateCase_CopyConstructionPreservesState
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidStateCase_CopyConstructionPreservesState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that copying preserves both source and destination states.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultVoidHasExactlyOneState_CopyConstructionPreservesState
        : public ::testing::TestWithParam<ResultVoidStateCase_CopyConstructionPreservesState>
    {
    };

    /* Verify that copying preserves both source and destination states.
     * 1. Arrange: Read the case parameters and construct the source result.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState_CopyConstructionPreservesState, CopyConstructionPreservesState)
    {
      /* Arrange */
      const ResultVoidStateCase_CopyConstructionPreservesState& parameter{GetParam()};
      const auto source =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      const Result<void> destination{source};

      /* Expect */
      EXPECT_EQ(destination.HasValue(), parameter.has_value);
      EXPECT_EQ(source.HasValue(), parameter.has_value);
    }

    std::string ResultVoidStateCaseName_CopyConstructionPreservesState(
      const ::testing::TestParamInfo<ResultVoidStateCase_CopyConstructionPreservesState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_005_ResultVoidHasExactlyOneState_CopyConstructionPreservesState,
      ::testing::Values(ResultVoidStateCase_CopyConstructionPreservesState{true, AdaptivePiErrc::kInvalidArgument,
                                                                           "Success",
                                                                           "Successful completion without a value"},
                        ResultVoidStateCase_CopyConstructionPreservesState{
                          false, AdaptivePiErrc::kInvalidArgument, "InvalidArgument", "Invalid argument failure"},
                        ResultVoidStateCase_CopyConstructionPreservesState{false, AdaptivePiErrc::kInvalidState,
                                                                           "InvalidState", "Invalid state failure"},
                        ResultVoidStateCase_CopyConstructionPreservesState{false, AdaptivePiErrc::kOperationFailed,
                                                                           "OperationFailed", "Operation failed"}),
      ResultVoidStateCaseName_CopyConstructionPreservesState);

    /* ----------------------------------------------------------------------------------- */

    /* Also exercises AP-R3-CORE-010. Do not observe the moved-from result. */
    struct ResultVoidStateCase_MoveConstructionPreservesState
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidStateCase_MoveConstructionPreservesState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that moving preserves the destination state without inspecting the
     * moved-from object. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultVoidHasExactlyOneState_MoveConstructionPreservesState
        : public ::testing::TestWithParam<ResultVoidStateCase_MoveConstructionPreservesState>
    {
    };

    /* Verify that moving preserves the destination state without inspecting the moved-from object.
     * 1. Arrange: Read the case parameters and construct the source result.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState_MoveConstructionPreservesState, MoveConstructionPreservesState)
    {
      /* Arrange */
      const ResultVoidStateCase_MoveConstructionPreservesState& parameter{GetParam()};
      auto source =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      /* Explicitly exercise move construction even when the payload is trivial. */
      /* NOLINTNEXTLINE(performance-move-const-arg) */
      const Result<void> destination{std::move(source)};

      /* Expect */
      EXPECT_EQ(destination.HasValue(), parameter.has_value);
    }

    std::string ResultVoidStateCaseName_MoveConstructionPreservesState(
      const ::testing::TestParamInfo<ResultVoidStateCase_MoveConstructionPreservesState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_005_ResultVoidHasExactlyOneState_MoveConstructionPreservesState,
      ::testing::Values(ResultVoidStateCase_MoveConstructionPreservesState{true, AdaptivePiErrc::kInvalidArgument,
                                                                           "Success",
                                                                           "Successful completion without a value"},
                        ResultVoidStateCase_MoveConstructionPreservesState{
                          false, AdaptivePiErrc::kInvalidArgument, "InvalidArgument", "Invalid argument failure"},
                        ResultVoidStateCase_MoveConstructionPreservesState{false, AdaptivePiErrc::kInvalidState,
                                                                           "InvalidState", "Invalid state failure"},
                        ResultVoidStateCase_MoveConstructionPreservesState{false, AdaptivePiErrc::kOperationFailed,
                                                                           "OperationFailed", "Operation failed"}),
      ResultVoidStateCaseName_MoveConstructionPreservesState);

    /* ----------------------------------------------------------------------------------- */

    struct ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure& parameter)
    {
      return output << parameter.description;
    }

    class VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure final
    {
      public:
        explicit VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure(
          const VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&) = delete;
        VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure(
          VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&& other) noexcept
            : VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure{other.live_count_}
        {
        }
        VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&
        operator=(const VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&) = delete;
        VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&
        operator=(VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure&&) = delete;

        ~VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    /* Provides the case data for this check: Verify construction and destruction of only the selected alternatives.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_005_ResultVoidHasExactlyOneState_ConstructsAnErrorOnlyOnFailure
        : public ::testing::TestWithParam<ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure>
    {
    };

    /* Verify construction and destruction of only the selected alternatives.
     * 1. Arrange: Initialize the lifetime counters and storage for observations.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct scoped results and record their state and live payload counts before destruction.
     * 3. Expect: Only selected payloads were alive and all live counts return to zero after scope exit.
     */
    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState_ConstructsAnErrorOnlyOnFailure, ConstructsAnErrorOnlyOnFailure)
    {
      /* Arrange */
      auto guard_probe = Result<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::FromValue();

      const ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure& parameter{GetParam()};
      int live_errors{0};
      bool selected_has_value{};
      int errors_while_alive{};

      /* Act */
      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      {
        const auto result = parameter.has_value
                              ? Result<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::FromValue()
                              : Result<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::FromError(
                                  VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure{live_errors});

        selected_has_value = result.HasValue();
        errors_while_alive = live_errors;
      }

      /* Expect */
      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_EQ(selected_has_value, parameter.has_value);
      EXPECT_EQ(errors_while_alive, parameter.has_value ? 0 : 1);

      EXPECT_EQ(live_errors, 0);
    }

    std::string ResultVoidStateCaseName_ConstructsAnErrorOnlyOnFailure(
      const ::testing::TestParamInfo<ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_005_ResultVoidHasExactlyOneState_ConstructsAnErrorOnlyOnFailure,
      ::testing::Values(ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure{true, AdaptivePiErrc::kInvalidArgument,
                                                                           "Success",
                                                                           "Successful completion without a value"},
                        ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure{
                          false, AdaptivePiErrc::kInvalidArgument, "InvalidArgument", "Invalid argument failure"},
                        ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure{false, AdaptivePiErrc::kInvalidState,
                                                                           "InvalidState", "Invalid state failure"},
                        ResultVoidStateCase_ConstructsAnErrorOnlyOnFailure{false, AdaptivePiErrc::kOperationFailed,
                                                                           "OperationFailed", "Operation failed"}),
      ResultVoidStateCaseName_ConstructsAnErrorOnlyOnFailure);

    /* ======================== End Test_AP_R3_CORE_005 ================================== */
    /* ========================== Test_AP_R3_CORE_006 ==================================== */

    using ConstructorResult = Result<int, AdaptivePiErrc>;

    /* ----------------------------------------------------------------------------------- */

    /* Verify an error convertible to the value type still requires explicit construction.
     * 1. Arrange: Choose distinct arithmetic value and error types with an implicit conversion.
     * 2. Act: Inspect implicit conversion and explicitly construct an error result.
     * 3. Expect: Implicit error conversion is forbidden and explicit construction selects error.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, ConvertibleErrorRequiresExplicitConstruction)
    {
      /* Arrange */
      using ConvertibleErrorResult = Result<long, int>;
      const int error{42};

      /* Act */
      constexpr bool implicitly_convertible = std::is_convertible_v<int, ConvertibleErrorResult>;
      constexpr bool lvalue_convertible = std::is_convertible_v<const int&, ConvertibleErrorResult>;
      const ConvertibleErrorResult result{error};

      /* Expect */
      EXPECT_FALSE(implicitly_convertible);
      EXPECT_FALSE(lvalue_convertible);
      ASSERT_FALSE(result.HasValue());
      EXPECT_EQ(result.Error(), error);
    }

    /* Values may convert implicitly; errors require explicit construction. */
    static_assert(std::is_convertible_v<int, ConstructorResult>);
    static_assert(std::is_constructible_v<ConstructorResult, const int&>);
    static_assert(std::is_constructible_v<ConstructorResult, const AdaptivePiErrc&>);
    static_assert(std::is_constructible_v<ConstructorResult, AdaptivePiErrc&&>);
    static_assert(!std::is_convertible_v<AdaptivePiErrc, ConstructorResult>);

    static_assert(std::is_constructible_v<Result<void, AdaptivePiErrc>, AdaptivePiErrc>);
    static_assert(!std::is_convertible_v<AdaptivePiErrc, Result<void, AdaptivePiErrc>>);

    /* ----------------------------------------------------------------------------------- */

    /* Verify direct construction from lvalue and rvalue value and error inputs.
     * 1. Arrange: Prepare an integer value and an AdaptivePi error enumeration.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, DirectConstructionSelectsValueOrError)
    {
      /* Arrange */
      const int value{42};
      const AdaptivePiErrc error{AdaptivePiErrc::kInvalidArgument};

      /* Act */
      const ConstructorResult copied_value{value};
      const ConstructorResult moved_value{42};
      const ConstructorResult copied_error{error};
      const ConstructorResult moved_error{AdaptivePiErrc::kOperationFailed};

      /* Expect */
      EXPECT_TRUE(copied_value.HasValue());
      EXPECT_TRUE(moved_value.HasValue());
      EXPECT_FALSE(copied_error.HasValue());
      EXPECT_FALSE(moved_error.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */

    class ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives final
    {
      public:
        explicit ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives(int& live_count)
            : live_count_{live_count}
        {
          ++live_count_;
        }

        ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives(
          const ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&) = delete;
        ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives(
          ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&& other) noexcept
            : ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives{other.live_count_}
        {
        }
        ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&
        operator=(const ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&) = delete;
        ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&
        operator=(ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives&&) = delete;

        ~ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    class TrackedError_DirectConstructionSupportsMoveOnlyAlternatives final
    {
      public:
        explicit TrackedError_DirectConstructionSupportsMoveOnlyAlternatives(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        TrackedError_DirectConstructionSupportsMoveOnlyAlternatives(
          const TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&) = delete;
        TrackedError_DirectConstructionSupportsMoveOnlyAlternatives(
          TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&& other) noexcept
            : TrackedError_DirectConstructionSupportsMoveOnlyAlternatives{other.live_count_}
        {
        }
        TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&
        operator=(const TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&) = delete;
        TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&
        operator=(TrackedError_DirectConstructionSupportsMoveOnlyAlternatives&&) = delete;

        ~TrackedError_DirectConstructionSupportsMoveOnlyAlternatives()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    /* Verify construction and destruction of only the selected alternatives.
     * 1. Arrange: Initialize the lifetime counters and storage for observations.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct scoped results and record their state and live payload counts before destruction.
     * 3. Expect: Only selected payloads were alive and all live counts return to zero after scope exit.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, DirectConstructionSupportsMoveOnlyAlternatives)
    {
      /* Arrange */
      int guard_live{0};
      auto guard_probe = Result<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
                                TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>::
        FromValue(ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives{guard_live});

      int live_values{0};
      int live_errors{0};
      bool value_has_value{};
      bool error_has_value{};
      int values_while_alive{};
      int errors_while_alive{};

      /* Act */
      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      {
        const Result<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
                     TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>
          value_result{ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives{live_values}};
        const Result<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
                     TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>
          error_result{TrackedError_DirectConstructionSupportsMoveOnlyAlternatives{live_errors}};

        value_has_value = value_result.HasValue();
        error_has_value = error_result.HasValue();
        values_while_alive = live_values;
        errors_while_alive = live_errors;
      }

      /* Expect */
      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_TRUE(value_has_value);
      EXPECT_FALSE(error_has_value);
      EXPECT_EQ(values_while_alive, 1);
      EXPECT_EQ(errors_while_alive, 1);

      EXPECT_EQ(live_values, 0);
      EXPECT_EQ(live_errors, 0);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify direct value construction and explicit error creation with identical types.
     * 1. Arrange: Prepare the integer payload shared by the value and error alternatives.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, SameTypesUseDirectConstructionForValue)
    {
      /* Arrange */
      const int payload{42};

      /* Act */
      const Result<int, int> value{payload};
      const auto error = Result<int, int>::FromError(payload);

      /* Expect */
      EXPECT_TRUE(value.HasValue());
      EXPECT_FALSE(error.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify direct error construction of a void Result.
     * 1. Arrange: Prepare the error enumeration used for lvalue construction.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, VoidResultSupportsDirectErrorConstruction)
    {
      /* Arrange */
      const AdaptivePiErrc error{AdaptivePiErrc::kInvalidArgument};

      /* Act */
      const Result<void, AdaptivePiErrc> copied_error{error};
      const Result<void, AdaptivePiErrc> moved_error{AdaptivePiErrc::kOperationFailed};

      /* Expect */
      EXPECT_FALSE(copied_error.HasValue());
      EXPECT_FALSE(moved_error.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */

    struct ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction
    {
        std::string text;
        int value;

        ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction(std::string text_in, int value_in)
            : text{std::move(text_in)}, value{value_in}
        {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
          /* Test-only fault injection for this exact constructor instantiation. */
          if (value_in < 0)
          {
            throw std::runtime_error{"in-place payload construction failed"};
          }
#endif
        }
    };

    /* Verify that factories construct the selected alternative from constructor arguments.
     * 1. Arrange: Prepare the text and integer arguments for the value payload.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct the results using the constructor or factory under test.
     * 3. Expect: The resulting objects report the intended value or error state.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, FactoryFunctionsSupportInPlaceConstruction)
    {
      /* Arrange */
      const std::string guard_text{"guard"};
      const int guard_value{0};
      auto guard_probe =
        Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::FromValue(
          guard_text, guard_value);

      const std::string text{"created-value"};
      const int value{42};

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      const int rejected_value{-1};
      bool caught_initial_failure{false};
#endif

      /* Act */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      try
      {
        (void)Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::FromValue(
          text, rejected_value);
      }
      catch (const std::runtime_error&)
      {
        caught_initial_failure = true;
      }
#endif
      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc> value_result =
        Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::FromValue(
          text, value);
      Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc> error_result =
        Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::FromError(
          AdaptivePiErrc::kInvalidArgument);

      /* Expect */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      EXPECT_TRUE(caught_initial_failure);
#endif
      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_TRUE(value_result.HasValue());
      EXPECT_FALSE(error_result.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */

    class ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative final
    {
      public:
        explicit ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative(int& live_count)
            : live_count_{live_count}
        {
          ++live_count_;
        }

        ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative(
          const ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&) = delete;
        ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative(
          ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&& other) noexcept
            : ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative{other.live_count_}
        {
        }
        ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&
        operator=(const ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&) = delete;
        ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&
        operator=(ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative&&) = delete;

        ~ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    class TrackedError_EmplaceValueAndErrorReplaceActiveAlternative final
    {
      public:
        explicit TrackedError_EmplaceValueAndErrorReplaceActiveAlternative(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        TrackedError_EmplaceValueAndErrorReplaceActiveAlternative(
          const TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&) = delete;
        TrackedError_EmplaceValueAndErrorReplaceActiveAlternative(
          TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&& other) noexcept
            : TrackedError_EmplaceValueAndErrorReplaceActiveAlternative{other.live_count_}
        {
        }
        TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&
        operator=(const TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&) = delete;
        TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&
        operator=(TrackedError_EmplaceValueAndErrorReplaceActiveAlternative&&) = delete;

        ~TrackedError_EmplaceValueAndErrorReplaceActiveAlternative()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    /* Verify that emplacement replaces the active alternative.
     * 1. Arrange: Construct an initial successful result and any lifetime counters.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Emplace an error and then a value, recording the state and live counts at each transition.
     * 3. Expect: Each replacement selects the requested state and destroys the previous payload.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, EmplaceValueAndErrorReplaceActiveAlternative)
    {
      /* Arrange */
      int guard_live{0};
      auto guard_probe = Result<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
                                TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>::
        FromValue(ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative{guard_live});

      int live_values{0};
      int live_errors{0};

      Result<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
             TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>
        result = Result<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
                        TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>::
          FromValue(ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative{live_values});

      /* Act */
      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      const bool initially_has_value = result.HasValue();

      result.EmplaceError(live_errors);
      const auto has_value_after_error = result.HasValue();
      const auto values_after_error = live_values;
      const auto errors_after_error = live_errors;

      result.EmplaceValue(live_values);
      const auto has_value_after_value = result.HasValue();
      const auto values_after_value = live_values;
      const auto errors_after_value = live_errors;

      /* Expect */
      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_TRUE(initially_has_value);
      EXPECT_FALSE(has_value_after_error);
      EXPECT_EQ(values_after_error, 0);
      EXPECT_EQ(errors_after_error, 1);
      EXPECT_TRUE(has_value_after_value);
      EXPECT_EQ(values_after_value, 1);
      EXPECT_EQ(errors_after_value, 0);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that emplacement replaces the active alternative.
     * 1. Arrange: Construct an initial successful result and any lifetime counters.
     * 2. Act: Emplace an error and then a value, recording the state and live counts at each transition.
     * 3. Expect: Each replacement selects the requested state and destroys the previous payload.
     */
    TEST(AP_R3_CORE_006_ResultCreationAndEmplacement, VoidResultSupportsEmplacement)
    {
      /* Arrange */
      Result<void, AdaptivePiErrc> result = Result<void, AdaptivePiErrc>::FromValue();

      /* Act */
      const bool initially_has_value = result.HasValue();

      result.EmplaceError(AdaptivePiErrc::kOperationFailed);
      const auto has_value_after_error = result.HasValue();

      result.EmplaceValue();
      const auto has_value_after_success = result.HasValue();

      /* Expect */
      EXPECT_TRUE(initially_has_value);
      EXPECT_FALSE(has_value_after_error);
      EXPECT_TRUE(has_value_after_success);
    }

    /* ----------------------------------------------------------------------------------- */

    /* All initial/replacement states, with and without an initial failed attempt. */
    /* Failed replacement must preserve CORE-005; our chosen policy also retains */
    /* the original payload and allows a subsequent retry. */
    struct ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess;

    struct PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess
    {
        const ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess* current{nullptr};
        int live{0};
    };

    struct ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess
    {
        PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess& observer;
        std::string text;
        int number;

        ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess(
          PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess& observer_in, std::string text_in,
          int number_in, bool fail)
            : observer{observer_in}, text{std::move(text_in)}, number{number_in}
        {
          if (fail)
          {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
            throw std::runtime_error{"payload construction failed"};
#else
            std::terminate();
#endif
          }
          observer.current = this;
          ++observer.live;
        }

        ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess(
          const ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&) = delete;
        ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess(
          ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&&) = delete;
        ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&
        operator=(const ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&) = delete;
        ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&
        operator=(ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess&&) = delete;

        ~ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess() noexcept
        {
          if (observer.current == this)
          {
            observer.current = nullptr;
          }
          --observer.live;
        }
    };

    struct ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess
    {
        bool starts_with_value;
        bool replaces_with_value;
        bool fail_first;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess& parameter)
    {
      return output << parameter.description;
    }

    struct ReplacementSnapshot_PreservesPayloadOnFailureAndReplacesOnSuccess
    {
        bool has_value{false};
        bool as_bool{false};
        bool original_present{false};
        bool original_unchanged{false};
        int original_live{0};
        std::string original_text;
        int original_number{0};
        bool replacement_present{false};
        int replacement_live{0};
        std::string replacement_text;
        int replacement_number{0};
    };

    /* Provides the case data for this check: Verify replacement, failure preservation, retry, and destruction across
     * the selected states. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_006_ReplacementTransitions
        : public ::testing::TestWithParam<ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess>
    {
    };

    /* Verify replacement, failure preservation, retry, and destruction across the selected states.
     * 1. Arrange: Create payload observers and storage for each state, content, identity, and lifetime checkpoint.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct the initial result, attempt the configured replacements, and record each checkpoint.
     * 3. Expect: The expected exception occurs, failure preserves the original payload, and success replaces and cleans
     * up payloads.
     */
    TEST_P(AP_R3_CORE_006_ReplacementTransitions, PreservesPayloadOnFailureAndReplacesOnSuccess)
    {
      /* Arrange */
      PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess guard_observer;
      auto guard_probe =
        Result<ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
               ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess>::FromValue(guard_observer, "original", 17,
                                                                                         false);

      const ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess& parameter{GetParam()};
      using ObservedResult = Result<ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
                                    ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess>;
      PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess original;
      PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess replacement;
      ReplacementSnapshot_PreservesPayloadOnFailureAndReplacesOnSuccess initial;
      ReplacementSnapshot_PreservesPayloadOnFailureAndReplacesOnSuccess failed;
      ReplacementSnapshot_PreservesPayloadOnFailureAndReplacesOnSuccess replaced;
      bool caught_failure{false};

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      PayloadObserver_PreservesPayloadOnFailureAndReplacesOnSuccess initial_failure_observer;
      bool caught_initial_fromvalue{false};
      bool caught_initial_fromerror{false};
#endif

      /* Act */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      try
      {
        (void)ObservedResult::FromValue(initial_failure_observer, "original", 17, true);
      }
      catch (const std::runtime_error&)
      {
        caught_initial_fromvalue = true;
      }
      try
      {
        (void)ObservedResult::FromError(initial_failure_observer, "original", 17, true);
      }
      catch (const std::runtime_error&)
      {
        caught_initial_fromerror = true;
      }
#endif

      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      {
        auto result = parameter.starts_with_value ? ObservedResult::FromValue(original, "original", 17, false)
                                                  : ObservedResult::FromError(original, "original", 17, false);
        const ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess* original_address{original.current};
        initial.has_value = result.HasValue();
        initial.as_bool = static_cast<bool>(result);
        initial.original_present = original.current != nullptr;
        initial.original_unchanged = original.current == original_address;
        initial.original_live = original.live;
        initial.original_text = original.current != nullptr ? original.current->text : std::string{};
        initial.original_number = original.current != nullptr ? original.current->number : 0;
        initial.replacement_present = replacement.current != nullptr;
        initial.replacement_live = replacement.live;
        initial.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
        initial.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
        if (parameter.fail_first)
        {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
          try
          {
            if (parameter.replaces_with_value)
            {
              result.EmplaceValue(replacement, "replacement", 42, true);
            }
            else
            {
              result.EmplaceError(replacement, "replacement", 42, true);
            }
          }
          catch (const std::runtime_error&)
          {
            caught_failure = true;
          }
#endif
          failed.has_value = result.HasValue();
          failed.as_bool = static_cast<bool>(result);
          failed.original_present = original.current != nullptr;
          failed.original_unchanged = original.current == original_address;
          failed.original_live = original.live;
          failed.original_text = original.current != nullptr ? original.current->text : std::string{};
          failed.original_number = original.current != nullptr ? original.current->number : 0;
          failed.replacement_present = replacement.current != nullptr;
          failed.replacement_live = replacement.live;
          failed.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
          failed.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
        }
        if (parameter.replaces_with_value)
        {
          result.EmplaceValue(replacement, "replacement", 42, false);
        }
        else
        {
          result.EmplaceError(replacement, "replacement", 42, false);
        }
        replaced.has_value = result.HasValue();
        replaced.as_bool = static_cast<bool>(result);
        replaced.original_present = original.current != nullptr;
        replaced.original_unchanged = original.current == original_address;
        replaced.original_live = original.live;
        replaced.original_text = original.current != nullptr ? original.current->text : std::string{};
        replaced.original_number = original.current != nullptr ? original.current->number : 0;
        replaced.replacement_present = replacement.current != nullptr;
        replaced.replacement_live = replacement.live;
        replaced.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
        replaced.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
      }

      /* Expect */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      EXPECT_TRUE(caught_initial_fromvalue);
      EXPECT_TRUE(caught_initial_fromerror);
      EXPECT_EQ(initial_failure_observer.live, 0);
      EXPECT_EQ(initial_failure_observer.current, nullptr);
#endif

      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_EQ(initial.has_value, parameter.starts_with_value);
      EXPECT_TRUE(initial.original_present);
      EXPECT_EQ(initial.original_text, "original");
      EXPECT_EQ(initial.original_number, 17);
      EXPECT_EQ(initial.original_live, 1);
      EXPECT_EQ(caught_failure, parameter.fail_first);
      if (parameter.fail_first)
      {
        EXPECT_EQ(failed.has_value, parameter.starts_with_value);
        /* AP-R3-CORE-007 also holds after failed replacement. */
        EXPECT_EQ(failed.as_bool, parameter.starts_with_value);
        EXPECT_TRUE(failed.original_unchanged);
        EXPECT_TRUE(failed.original_present);
        EXPECT_EQ(failed.original_live, 1);
        EXPECT_EQ(failed.original_text, "original");
        EXPECT_EQ(failed.original_number, 17);
        EXPECT_FALSE(failed.replacement_present);
        EXPECT_EQ(failed.replacement_live, 0);
      }
      EXPECT_EQ(replaced.has_value, parameter.replaces_with_value);
      EXPECT_FALSE(replaced.original_present);
      EXPECT_EQ(replaced.original_live, 0);
      EXPECT_TRUE(replaced.replacement_present);
      EXPECT_EQ(replaced.replacement_text, "replacement");
      EXPECT_EQ(replaced.replacement_number, 42);
      EXPECT_EQ(replaced.replacement_live, 1);
      EXPECT_EQ(original.live, 0);
      EXPECT_EQ(replacement.live, 0);
      EXPECT_EQ(replacement.current, nullptr);
    }

    std::string ReplacementCaseName_PreservesPayloadOnFailureAndReplacesOnSuccess(
      const ::testing::TestParamInfo<ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess>& information)
    {
      return std::string{information.param.test_name};
    }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    INSTANTIATE_TEST_SUITE_P(
      AllStates, AP_R3_CORE_006_ReplacementTransitions,
      ::testing::Values(
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{false, false, false, "ErrorToErrorSuccess",
                                                                      "error to error with immediate success"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{false, true, false, "ErrorToValueSuccess",
                                                                      "error to value with immediate success"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{true, false, false, "ValueToErrorSuccess",
                                                                      "value to error with immediate success"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{true, true, false, "ValueToValueSuccess",
                                                                      "value to value with immediate success"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{false, false, true, "ErrorToErrorAfterFailure",
                                                                      "error to error after a failed replacement"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{false, true, true, "ErrorToValueAfterFailure",
                                                                      "error to value after a failed replacement"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{true, false, true, "ValueToErrorAfterFailure",
                                                                      "value to error after a failed replacement"},
        ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{true, true, true, "ValueToValueAfterFailure",
                                                                      "value to value after a failed replacement"}),
      ReplacementCaseName_PreservesPayloadOnFailureAndReplacesOnSuccess);
#else
    INSTANTIATE_TEST_SUITE_P(AllStates, AP_R3_CORE_006_ReplacementTransitions,
                             ::testing::Values(
                               ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{
                                 false, false, false, "ErrorToErrorSuccess", "error to error with immediate success"},
                               ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{
                                 false, true, false, "ErrorToValueSuccess", "error to value with immediate success"},
                               ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{
                                 true, false, false, "ValueToErrorSuccess", "value to error with immediate success"},
                               ReplacementCase_PreservesPayloadOnFailureAndReplacesOnSuccess{
                                 true, true, false, "ValueToValueSuccess", "value to value with immediate success"}),
                             ReplacementCaseName_PreservesPayloadOnFailureAndReplacesOnSuccess);
#endif

    /* ----------------------------------------------------------------------------------- */

    struct VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess;

    struct VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess
    {
        const VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess* current{nullptr};
        int live{0};
    };

    struct VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess
    {
        VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess& observer;
        std::string text;
        int number;

        VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess(
          VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess& observer_in, std::string text_in,
          int number_in, bool fail)
            : observer{observer_in}, text{std::move(text_in)}, number{number_in}
        {
          if (fail)
          {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
            throw std::runtime_error{"payload construction failed"};
#else
            std::terminate();
#endif
          }
          observer.current = this;
          ++observer.live;
        }

        VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess(
          const VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&) = delete;
        VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess(
          VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&&) = delete;
        VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&
        operator=(const VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&) = delete;
        VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&
        operator=(VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess&&) = delete;

        ~VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess() noexcept
        {
          if (observer.current == this)
          {
            observer.current = nullptr;
          }
          --observer.live;
        }
    };

    struct VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess
    {
        bool starts_with_value;
        bool fail_first;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output,
                             const VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess& parameter)
    {
      return output << parameter.description;
    }

    struct VoidReplacementSnapshot_PreservesStateOnFailureAndReplacesOnSuccess
    {
        bool has_value{false};
        bool as_bool{false};
        bool original_present{false};
        bool original_unchanged{false};
        int original_live{0};
        std::string original_text;
        int original_number{0};
        bool replacement_present{false};
        int replacement_live{0};
        std::string replacement_text;
        int replacement_number{0};
    };

    /* Provides the case data for this check: Verify replacement, failure preservation, retry, and destruction across
     * the selected states. GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_006_VoidReplacement
        : public ::testing::TestWithParam<VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess>
    {
    };

    /* Verify replacement, failure preservation, retry, and destruction across the selected states.
     * 1. Arrange: Create payload observers and storage for each state, content, identity, and lifetime checkpoint.
     *    A separate coverage-only probe permits child-only empty-slot fault injection.
     * 2. Act: Construct the initial result, attempt the configured replacements, and record each checkpoint.
     * 3. Expect: The expected exception occurs, failure preserves the original payload, and success replaces and cleans
     * up payloads.
     */
    TEST_P(AP_R3_CORE_006_VoidReplacement, PreservesStateOnFailureAndReplacesOnSuccess)
    {
      /* Arrange */
      auto guard_probe = Result<void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess>::FromValue();

      const VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess& parameter{GetParam()};
      using ObservedResult = Result<void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess>;
      VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess original;
      VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess replacement;
      VoidReplacementSnapshot_PreservesStateOnFailureAndReplacesOnSuccess initial;
      VoidReplacementSnapshot_PreservesStateOnFailureAndReplacesOnSuccess failed;
      VoidReplacementSnapshot_PreservesStateOnFailureAndReplacesOnSuccess replaced;
      bool caught_failure{false};
      bool success_after_error{false};
      bool repeated_success{false};
      int live_after_success{0};
      bool payload_after_success{false};

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      VoidPayloadObserver_PreservesStateOnFailureAndReplacesOnSuccess initial_failure_observer;
      bool caught_initial_fromerror{false};
#endif

      /* Act */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      try
      {
        (void)ObservedResult::FromError(initial_failure_observer, "original", 17, true);
      }
      catch (const std::runtime_error&)
      {
        caught_initial_fromerror = true;
      }
#endif

      /* Coverage-only fault injection: a live Result cannot lose its active slot
       * through the public API. Force it here to exercise this payload's guard.
       */
      const auto query_empty_slot = [&guard_probe]()
      {
        SlotsOf(StorageOf(guard_probe))[ActiveIndexOf(StorageOf(guard_probe))].reset();
        (void)guard_probe.HasValue();
      };
      {
        auto result = parameter.starts_with_value ? ObservedResult::FromValue()
                                                  : ObservedResult::FromError(original, "original", 17, false);
        const VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess* original_address{original.current};
        initial.has_value = result.HasValue();
        initial.as_bool = static_cast<bool>(result);
        initial.original_present = original.current != nullptr;
        initial.original_unchanged = original.current == original_address;
        initial.original_live = original.live;
        initial.original_text = original.current != nullptr ? original.current->text : std::string{};
        initial.original_number = original.current != nullptr ? original.current->number : 0;
        initial.replacement_present = replacement.current != nullptr;
        initial.replacement_live = replacement.live;
        initial.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
        initial.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
        if (parameter.fail_first)
        {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
          try
          {
            result.EmplaceError(replacement, "replacement", 42, true);
          }
          catch (const std::runtime_error&)
          {
            caught_failure = true;
          }
#endif
          failed.has_value = result.HasValue();
          failed.as_bool = static_cast<bool>(result);
          failed.original_present = original.current != nullptr;
          failed.original_unchanged = original.current == original_address;
          failed.original_live = original.live;
          failed.original_text = original.current != nullptr ? original.current->text : std::string{};
          failed.original_number = original.current != nullptr ? original.current->number : 0;
          failed.replacement_present = replacement.current != nullptr;
          failed.replacement_live = replacement.live;
          failed.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
          failed.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
        }
        result.EmplaceError(replacement, "replacement", 42, false);
        replaced.has_value = result.HasValue();
        replaced.as_bool = static_cast<bool>(result);
        replaced.original_present = original.current != nullptr;
        replaced.original_unchanged = original.current == original_address;
        replaced.original_live = original.live;
        replaced.original_text = original.current != nullptr ? original.current->text : std::string{};
        replaced.original_number = original.current != nullptr ? original.current->number : 0;
        replaced.replacement_present = replacement.current != nullptr;
        replaced.replacement_live = replacement.live;
        replaced.replacement_text = replacement.current != nullptr ? replacement.current->text : std::string{};
        replaced.replacement_number = replacement.current != nullptr ? replacement.current->number : 0;
        result.EmplaceValue();
        success_after_error = result.HasValue();
        live_after_success = replacement.live;
        payload_after_success = replacement.current != nullptr;
        result.EmplaceValue();
        repeated_success = result.HasValue();
      }

      /* Expect */
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      EXPECT_TRUE(caught_initial_fromerror);
      EXPECT_EQ(initial_failure_observer.live, 0);
      EXPECT_EQ(initial_failure_observer.current, nullptr);
#endif

      EXPECT_DEATH(query_empty_slot(), "");
      EXPECT_EQ(initial.has_value, parameter.starts_with_value);
      EXPECT_EQ(initial.original_present, !parameter.starts_with_value);
      if (!parameter.starts_with_value)
      {
        EXPECT_EQ(initial.original_text, "original");
        EXPECT_EQ(initial.original_number, 17);
      }
      EXPECT_EQ(caught_failure, parameter.fail_first);
      if (parameter.fail_first)
      {
        EXPECT_EQ(failed.has_value, parameter.starts_with_value);
        /* AP-R3-CORE-007 also holds after failed replacement. */
        EXPECT_EQ(failed.as_bool, parameter.starts_with_value);
        EXPECT_TRUE(failed.original_unchanged);
        EXPECT_EQ(failed.original_live, parameter.starts_with_value ? 0 : 1);
        EXPECT_EQ(failed.original_present, !parameter.starts_with_value);
        if (!parameter.starts_with_value)
        {
          EXPECT_EQ(failed.original_text, "original");
          EXPECT_EQ(failed.original_number, 17);
        }
        EXPECT_FALSE(failed.replacement_present);
        EXPECT_EQ(failed.replacement_live, 0);
      }
      EXPECT_FALSE(replaced.has_value);
      EXPECT_FALSE(replaced.original_present);
      EXPECT_EQ(replaced.original_live, 0);
      EXPECT_TRUE(replaced.replacement_present);
      EXPECT_EQ(replaced.replacement_text, "replacement");
      EXPECT_EQ(replaced.replacement_number, 42);
      EXPECT_EQ(replaced.replacement_live, 1);
      EXPECT_TRUE(success_after_error);
      EXPECT_EQ(live_after_success, 0);
      EXPECT_FALSE(payload_after_success);
      EXPECT_TRUE(repeated_success);
      EXPECT_EQ(original.live, 0);
      EXPECT_EQ(replacement.live, 0);
      EXPECT_EQ(replacement.current, nullptr);
    }

    std::string VoidReplacementCaseName_PreservesStateOnFailureAndReplacesOnSuccess(
      const ::testing::TestParamInfo<VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess>& information)
    {
      return std::string{information.param.test_name};
    }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    INSTANTIATE_TEST_SUITE_P(
      AllStates, AP_R3_CORE_006_VoidReplacement,
      ::testing::Values(
        VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{false, false, "ErrorToErrorSuccess",
                                                                        "error to error with immediate success"},
        VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{true, false, "SuccessToErrorSuccess",
                                                                        "success to error with immediate success"},
        VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{false, true, "ErrorToErrorAfterFailure",
                                                                        "error to error after a failed replacement"},
        VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{true, true, "SuccessToErrorAfterFailure",
                                                                        "success to error after a failed replacement"}),
      VoidReplacementCaseName_PreservesStateOnFailureAndReplacesOnSuccess);
#else
    INSTANTIATE_TEST_SUITE_P(AllStates, AP_R3_CORE_006_VoidReplacement,
                             ::testing::Values(
                               VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{
                                 false, false, "ErrorToErrorSuccess", "error to error with immediate success"},
                               VoidReplacementCase_PreservesStateOnFailureAndReplacesOnSuccess{
                                 true, false, "SuccessToErrorSuccess", "success to error with immediate success"}),
                             VoidReplacementCaseName_PreservesStateOnFailureAndReplacesOnSuccess);
#endif

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    /* ----------------------------------------------------------------------------------- */

    /* Verify initial FromValue construction propagates failure and releases acquired resources.
     * 1. Arrange: Define a payload that acquires ownership before its constructor throws.
     * 2. Act: Transfer ownership, catch the failure, then retry with a non-failing payload.
     * 3. Expect: The exception propagates; failed and successful construction both release their resources.
     */
    TEST(AP_R3_CORE_006_InitialConstructionFailure, ValueFactoryPropagatesConstructionFailure)
    {
      /* Arrange */
      struct FailingPayload
      {
          std::shared_ptr<int> resource;

          explicit FailingPayload(std::shared_ptr<int> input) : resource{std::move(input)}
          {
            if (*resource == 42)
            {
              throw std::runtime_error{"initial construction failed"};
            }
          }
      };
      auto resource = std::make_shared<int>(42);
      const std::weak_ptr<int> observer{resource};
      bool caught_failure{false};
      std::string message;
      auto retry_resource = std::make_shared<int>(43);
      const std::weak_ptr<int> retry_observer{retry_resource};
      bool retry_owned_resource{false};

      /* Act */
      try
      {
        (void)Result<FailingPayload, int>::FromValue(std::move(resource));
      }
      catch (const std::runtime_error& error)
      {
        caught_failure = true;
        message = error.what();
      }

      {
        const auto retry = Result<FailingPayload, int>::FromValue(std::move(retry_resource));
        retry_owned_resource = !retry_observer.expired();
      }

      /* Expect */
      EXPECT_TRUE(caught_failure);
      EXPECT_EQ(message, "initial construction failed");
      EXPECT_TRUE(observer.expired());
      EXPECT_TRUE(retry_owned_resource);
      EXPECT_TRUE(retry_observer.expired());
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify initial FromError construction propagates failure and releases acquired resources.
     * 1. Arrange: Define a payload that acquires ownership before its constructor throws.
     * 2. Act: Transfer ownership, catch the failure, then retry with a non-failing payload.
     * 3. Expect: The exception propagates; failed and successful construction both release their resources.
     */
    TEST(AP_R3_CORE_006_InitialConstructionFailure, ErrorFactoryPropagatesConstructionFailure)
    {
      /* Arrange */
      struct FailingPayload
      {
          std::shared_ptr<int> resource;

          explicit FailingPayload(std::shared_ptr<int> input) : resource{std::move(input)}
          {
            if (*resource == 42)
            {
              throw std::runtime_error{"initial construction failed"};
            }
          }
      };
      auto resource = std::make_shared<int>(42);
      const std::weak_ptr<int> observer{resource};
      bool caught_failure{false};
      std::string message;
      auto retry_resource = std::make_shared<int>(43);
      const std::weak_ptr<int> retry_observer{retry_resource};
      bool retry_owned_resource{false};

      /* Act */
      try
      {
        (void)Result<int, FailingPayload>::FromError(std::move(resource));
      }
      catch (const std::runtime_error& error)
      {
        caught_failure = true;
        message = error.what();
      }

      {
        const auto retry = Result<int, FailingPayload>::FromError(std::move(retry_resource));
        retry_owned_resource = !retry_observer.expired();
      }

      /* Expect */
      EXPECT_TRUE(caught_failure);
      EXPECT_EQ(message, "initial construction failed");
      EXPECT_TRUE(observer.expired());
      EXPECT_TRUE(retry_owned_resource);
      EXPECT_TRUE(retry_observer.expired());
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify initial FromError construction propagates failure and releases acquired resources.
     * 1. Arrange: Define a payload that acquires ownership before its constructor throws.
     * 2. Act: Transfer ownership, catch the failure, then retry with a non-failing payload.
     * 3. Expect: The exception propagates; failed and successful construction both release their resources.
     */
    TEST(AP_R3_CORE_006_InitialConstructionFailure, VoidErrorFactoryPropagatesConstructionFailure)
    {
      /* Arrange */
      struct FailingPayload
      {
          std::shared_ptr<int> resource;

          explicit FailingPayload(std::shared_ptr<int> input) : resource{std::move(input)}
          {
            if (*resource == 42)
            {
              throw std::runtime_error{"initial construction failed"};
            }
          }
      };
      auto resource = std::make_shared<int>(42);
      const std::weak_ptr<int> observer{resource};
      bool caught_failure{false};
      std::string message;
      auto retry_resource = std::make_shared<int>(43);
      const std::weak_ptr<int> retry_observer{retry_resource};
      bool retry_owned_resource{false};

      /* Act */
      try
      {
        (void)Result<void, FailingPayload>::FromError(std::move(resource));
      }
      catch (const std::runtime_error& error)
      {
        caught_failure = true;
        message = error.what();
      }

      {
        const auto retry = Result<void, FailingPayload>::FromError(std::move(retry_resource));
        retry_owned_resource = !retry_observer.expired();
      }

      /* Expect */
      EXPECT_TRUE(caught_failure);
      EXPECT_EQ(message, "initial construction failed");
      EXPECT_TRUE(observer.expired());
      EXPECT_TRUE(retry_owned_resource);
      EXPECT_TRUE(retry_observer.expired());
    }

#endif

    /* ======================== End Test_AP_R3_CORE_006 ================================== */

    /* ========================== Test_AP_R3_CORE_007 ==================================== */

    /* ----------------------------------------------------------------------------------- */

    struct ResultQueryCase_ReportsSelectedState
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultQueryCase_ReportsSelectedState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that the selected result state is reported correctly.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_007_HasValueAndBoolReportState_ValueStates
        : public ::testing::TestWithParam<ResultQueryCase_ReportsSelectedState>
    {
    };

    /* Verify that the selected result state is reported correctly.
     * 1. Arrange: Read the case parameters and prepare the selected result state.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST_P(AP_R3_CORE_007_HasValueAndBoolReportState_ValueStates, ReportsSelectedState)
    {
      /* Arrange */
      const auto& parameter = GetParam();
      const auto result = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                              : Result<int>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, parameter.has_value);
      EXPECT_EQ(state_1_as_bool, parameter.has_value);
      EXPECT_EQ(state_1_entered_success, parameter.has_value);
      EXPECT_EQ(state_1_repeated_has_value, parameter.has_value);
      EXPECT_EQ(state_1_repeated_as_bool, parameter.has_value);
    }

    std::string ResultQueryCaseName_ReportsSelectedState(
      const ::testing::TestParamInfo<ResultQueryCase_ReportsSelectedState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_007_HasValueAndBoolReportState_ValueStates,
      ::testing::Values(ResultQueryCase_ReportsSelectedState{true, 0, AdaptivePiErrc::kInvalidArgument, "ZeroValue",
                                                             "Zero is a successful value"},
                        ResultQueryCase_ReportsSelectedState{true, 42, AdaptivePiErrc::kInvalidState, "PositiveValue",
                                                             "Positive integer success"},
                        ResultQueryCase_ReportsSelectedState{true, -1, AdaptivePiErrc::kOperationFailed,
                                                             "NegativeValue", "Negative integer success"},
                        ResultQueryCase_ReportsSelectedState{false, 0, AdaptivePiErrc::kInvalidArgument,
                                                             "InvalidArgument", "Invalid argument failure"},
                        ResultQueryCase_ReportsSelectedState{false, 0, AdaptivePiErrc::kInvalidState, "InvalidState",
                                                             "Invalid state failure"},
                        ResultQueryCase_ReportsSelectedState{false, 0, AdaptivePiErrc::kOperationFailed,
                                                             "OperationFailed", "Operation failed"}),
      ResultQueryCaseName_ReportsSelectedState);

    /* ----------------------------------------------------------------------------------- */

    struct ResultVoidQueryCase_ReportsSelectedState
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidQueryCase_ReportsSelectedState& parameter)
    {
      return output << parameter.description;
    }

    /* Provides the case data for this check: Verify that the selected result state is reported correctly.
     * GetParam() supplies this test's inputs and expected results.
     */
    class AP_R3_CORE_007_HasValueAndBoolReportState_VoidStates
        : public ::testing::TestWithParam<ResultVoidQueryCase_ReportsSelectedState>
    {
    };

    /* Verify that the selected result state is reported correctly.
     * 1. Arrange: Read the case parameters and prepare the selected result state.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST_P(AP_R3_CORE_007_HasValueAndBoolReportState_VoidStates, ReportsSelectedState)
    {
      /* Arrange */
      const auto& parameter = GetParam();
      const auto result =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, parameter.has_value);
      EXPECT_EQ(state_1_as_bool, parameter.has_value);
      EXPECT_EQ(state_1_entered_success, parameter.has_value);
      EXPECT_EQ(state_1_repeated_has_value, parameter.has_value);
      EXPECT_EQ(state_1_repeated_as_bool, parameter.has_value);
    }

    std::string ResultVoidQueryCaseName_ReportsSelectedState(
      const ::testing::TestParamInfo<ResultVoidQueryCase_ReportsSelectedState>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_007_HasValueAndBoolReportState_VoidStates,
      ::testing::Values(ResultVoidQueryCase_ReportsSelectedState{true, AdaptivePiErrc::kInvalidArgument, "Success",
                                                                 "Successful completion without a value"},
                        ResultVoidQueryCase_ReportsSelectedState{false, AdaptivePiErrc::kInvalidArgument,
                                                                 "InvalidArgument", "Invalid argument failure"},
                        ResultVoidQueryCase_ReportsSelectedState{false, AdaptivePiErrc::kInvalidState, "InvalidState",
                                                                 "Invalid state failure"},
                        ResultVoidQueryCase_ReportsSelectedState{false, AdaptivePiErrc::kOperationFailed,
                                                                 "OperationFailed", "Operation failed"}),
      ResultVoidQueryCaseName_ReportsSelectedState);

    /* ----------------------------------------------------------------------------------- */

    /* Verify that a false boolean payload still represents success.
     * 1. Arrange: Create a successful Result<bool> containing false.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST(AP_R3_CORE_007_HasValueAndBoolReportState, FalsePayloadReportsSuccess)
    {
      /* Arrange */
      const auto result = Result<bool>::FromValue(false);

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, true);
      EXPECT_EQ(state_1_as_bool, true);
      EXPECT_EQ(state_1_entered_success, true);
      EXPECT_EQ(state_1_repeated_has_value, true);
      EXPECT_EQ(state_1_repeated_as_bool, true);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that equal integer payloads retain distinct value and error states.
     * 1. Arrange: Create a value result and an error result, both containing zero.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST(AP_R3_CORE_007_HasValueAndBoolReportState, IdenticalPayloadsReportDifferentStates)
    {
      /* Arrange */
      const auto value = Result<int, int>::FromValue(0);
      const auto error = Result<int, int>::FromError(0);

      /* Act */
      const bool state_1_has_value = value.HasValue();
      const bool state_1_as_bool = static_cast<bool>(value);
      bool state_1_entered_success{false};
      if (value)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = value.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(value);
      const bool state_2_has_value = error.HasValue();
      const bool state_2_as_bool = static_cast<bool>(error);
      bool state_2_entered_success{false};
      if (error)
      {
        state_2_entered_success = true;
      }
      const bool state_2_repeated_has_value = error.HasValue();
      const bool state_2_repeated_as_bool = static_cast<bool>(error);

      /* Expect */
      EXPECT_EQ(state_1_has_value, true);
      EXPECT_EQ(state_1_as_bool, true);
      EXPECT_EQ(state_1_entered_success, true);
      EXPECT_EQ(state_1_repeated_has_value, true);
      EXPECT_EQ(state_1_repeated_as_bool, true);
      EXPECT_EQ(state_2_has_value, false);
      EXPECT_EQ(state_2_as_bool, false);
      EXPECT_EQ(state_2_entered_success, false);
      EXPECT_EQ(state_2_repeated_has_value, false);
      EXPECT_EQ(state_2_repeated_as_bool, false);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that a void result containing a zero error still represents failure.
     * 1. Arrange: Create a Result<void, int> containing error zero.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST(AP_R3_CORE_007_HasValueAndBoolReportState, VoidZeroErrorReportsFailure)
    {
      /* Arrange */
      const auto result = Result<void, int>::FromError(0);

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, false);
      EXPECT_EQ(state_1_as_bool, false);
      EXPECT_EQ(state_1_entered_success, false);
      EXPECT_EQ(state_1_repeated_has_value, false);
      EXPECT_EQ(state_1_repeated_as_bool, false);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify state queries after repeated value and error replacements.
     * 1. Arrange: Create an integer result initially containing value zero.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST(AP_R3_CORE_007_HasValueAndBoolReportState, QueriesFollowValueReplacement)
    {
      /* Arrange */
      auto result = Result<int, int>::FromValue(0);

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceValue(-1);
      const bool state_2_has_value = result.HasValue();
      const bool state_2_as_bool = static_cast<bool>(result);
      bool state_2_entered_success{false};
      if (result)
      {
        state_2_entered_success = true;
      }
      const bool state_2_repeated_has_value = result.HasValue();
      const bool state_2_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceError(0);
      const bool state_3_has_value = result.HasValue();
      const bool state_3_as_bool = static_cast<bool>(result);
      bool state_3_entered_success{false};
      if (result)
      {
        state_3_entered_success = true;
      }
      const bool state_3_repeated_has_value = result.HasValue();
      const bool state_3_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceError(42);
      const bool state_4_has_value = result.HasValue();
      const bool state_4_as_bool = static_cast<bool>(result);
      bool state_4_entered_success{false};
      if (result)
      {
        state_4_entered_success = true;
      }
      const bool state_4_repeated_has_value = result.HasValue();
      const bool state_4_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceValue(0);
      const bool state_5_has_value = result.HasValue();
      const bool state_5_as_bool = static_cast<bool>(result);
      bool state_5_entered_success{false};
      if (result)
      {
        state_5_entered_success = true;
      }
      const bool state_5_repeated_has_value = result.HasValue();
      const bool state_5_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, true);
      EXPECT_EQ(state_1_as_bool, true);
      EXPECT_EQ(state_1_entered_success, true);
      EXPECT_EQ(state_1_repeated_has_value, true);
      EXPECT_EQ(state_1_repeated_as_bool, true);
      EXPECT_EQ(state_2_has_value, true);
      EXPECT_EQ(state_2_as_bool, true);
      EXPECT_EQ(state_2_entered_success, true);
      EXPECT_EQ(state_2_repeated_has_value, true);
      EXPECT_EQ(state_2_repeated_as_bool, true);
      EXPECT_EQ(state_3_has_value, false);
      EXPECT_EQ(state_3_as_bool, false);
      EXPECT_EQ(state_3_entered_success, false);
      EXPECT_EQ(state_3_repeated_has_value, false);
      EXPECT_EQ(state_3_repeated_as_bool, false);
      EXPECT_EQ(state_4_has_value, false);
      EXPECT_EQ(state_4_as_bool, false);
      EXPECT_EQ(state_4_entered_success, false);
      EXPECT_EQ(state_4_repeated_has_value, false);
      EXPECT_EQ(state_4_repeated_as_bool, false);
      EXPECT_EQ(state_5_has_value, true);
      EXPECT_EQ(state_5_as_bool, true);
      EXPECT_EQ(state_5_entered_success, true);
      EXPECT_EQ(state_5_repeated_has_value, true);
      EXPECT_EQ(state_5_repeated_as_bool, true);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify void state queries after repeated success and error replacements.
     * 1. Arrange: Create a void result initially representing success.
     * 2. Act: Query HasValue, bool conversion, and conditional branching, including after any replacements.
     * 3. Expect: Every observation and repeated query matches the active alternative, regardless of payload.
     */
    TEST(AP_R3_CORE_007_HasValueAndBoolReportState, QueriesFollowVoidReplacement)
    {
      /* Arrange */
      auto result = Result<void, int>::FromValue();

      /* Act */
      const bool state_1_has_value = result.HasValue();
      const bool state_1_as_bool = static_cast<bool>(result);
      bool state_1_entered_success{false};
      if (result)
      {
        state_1_entered_success = true;
      }
      const bool state_1_repeated_has_value = result.HasValue();
      const bool state_1_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceValue();
      const bool state_2_has_value = result.HasValue();
      const bool state_2_as_bool = static_cast<bool>(result);
      bool state_2_entered_success{false};
      if (result)
      {
        state_2_entered_success = true;
      }
      const bool state_2_repeated_has_value = result.HasValue();
      const bool state_2_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceError(0);
      const bool state_3_has_value = result.HasValue();
      const bool state_3_as_bool = static_cast<bool>(result);
      bool state_3_entered_success{false};
      if (result)
      {
        state_3_entered_success = true;
      }
      const bool state_3_repeated_has_value = result.HasValue();
      const bool state_3_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceError(42);
      const bool state_4_has_value = result.HasValue();
      const bool state_4_as_bool = static_cast<bool>(result);
      bool state_4_entered_success{false};
      if (result)
      {
        state_4_entered_success = true;
      }
      const bool state_4_repeated_has_value = result.HasValue();
      const bool state_4_repeated_as_bool = static_cast<bool>(result);
      result.EmplaceValue();
      const bool state_5_has_value = result.HasValue();
      const bool state_5_as_bool = static_cast<bool>(result);
      bool state_5_entered_success{false};
      if (result)
      {
        state_5_entered_success = true;
      }
      const bool state_5_repeated_has_value = result.HasValue();
      const bool state_5_repeated_as_bool = static_cast<bool>(result);

      /* Expect */
      EXPECT_EQ(state_1_has_value, true);
      EXPECT_EQ(state_1_as_bool, true);
      EXPECT_EQ(state_1_entered_success, true);
      EXPECT_EQ(state_1_repeated_has_value, true);
      EXPECT_EQ(state_1_repeated_as_bool, true);
      EXPECT_EQ(state_2_has_value, true);
      EXPECT_EQ(state_2_as_bool, true);
      EXPECT_EQ(state_2_entered_success, true);
      EXPECT_EQ(state_2_repeated_has_value, true);
      EXPECT_EQ(state_2_repeated_as_bool, true);
      EXPECT_EQ(state_3_has_value, false);
      EXPECT_EQ(state_3_as_bool, false);
      EXPECT_EQ(state_3_entered_success, false);
      EXPECT_EQ(state_3_repeated_has_value, false);
      EXPECT_EQ(state_3_repeated_as_bool, false);
      EXPECT_EQ(state_4_has_value, false);
      EXPECT_EQ(state_4_as_bool, false);
      EXPECT_EQ(state_4_entered_success, false);
      EXPECT_EQ(state_4_repeated_has_value, false);
      EXPECT_EQ(state_4_repeated_as_bool, false);
      EXPECT_EQ(state_5_has_value, true);
      EXPECT_EQ(state_5_as_bool, true);
      EXPECT_EQ(state_5_entered_success, true);
      EXPECT_EQ(state_5_repeated_has_value, true);
      EXPECT_EQ(state_5_repeated_as_bool, true);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_007_EmptySlotDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_007_EmptySlotDeathTestTypes =
      ::testing::Types<Result<int>, Result<int, int>, Result<void>, Result<long, int>, Result<int, AdaptivePiErrc>,
                       Result<void, AdaptivePiErrc>, Result<bool>, Result<void, int>, Result<std::string>,
                       Result<std::unique_ptr<int>>, Result<int, std::unique_ptr<int>>,
                       Result<void, std::unique_ptr<int>>>;
    struct AP_R3_CORE_007_EmptySlotDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 12> names{
            "IntDefaultError",  "IntInt",  "VoidDefaultError",   "LongInt",       "IntEnum",       "VoidEnum",
            "BoolDefaultError", "VoidInt", "StringDefaultError", "MoveOnlyValue", "MoveOnlyError", "VoidMoveOnlyError"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_007_EmptySlotDeathTest, AP_R3_CORE_007_EmptySlotDeathTestTypes,
                     AP_R3_CORE_007_EmptySlotDeathTestNames);

    /* Exercise index()'s otherwise infeasible empty-slot branch for each payload.
     * Coverage-only: construction and replacement always leave an engaged active slot.
     * 1. Arrange: Construct a valid success result of the selected type.
     * 2. Act: Prepare a child-only mutation that clears its active optional before querying.
     * 3. Expect: The corrupted child terminates and the parent's success state is intact. */
    TYPED_TEST(AP_R3_CORE_007_EmptySlotDeathTest, ForcedEmptySlotTerminates)
    {
      /* Arrange */
      auto result = TypeParam::FromValue();

      /* Act */
      const auto query = [&result]()
      {
        SlotsOf(StorageOf(result))[ActiveIndexOf(StorageOf(result))].reset();
        (void)result.HasValue();
      };

      /* Expect */
      EXPECT_DEATH(query(), "");
      EXPECT_TRUE(result.HasValue());
    }

    /* ======================== End Test_AP_R3_CORE_007 ================================== */
    /* =============================== Test_AP_R3_CORE_008 =============================== */

    template <typename R, typename = void>
    struct HasValueOrThrow008 : std::false_type
    {
    };
    template <typename R>
    struct HasValueOrThrow008<R, std::void_t<decltype(std::declval<R>().ValueOrThrow())>> : std::true_type
    {
    };
    static_assert(!HasValueOrThrow008<Result<int, int>&>::value);
    static_assert(!HasValueOrThrow008<const Result<int, int>&>::value);
    static_assert(!HasValueOrThrow008<Result<int, int>&&>::value);
    static_assert(!HasValueOrThrow008<Result<void, int>&>::value);
    static_assert(!HasValueOrThrow008<const Result<void, int>&>::value);
    static_assert(!HasValueOrThrow008<Result<void, int>&&>::value);
    template <typename R, typename = void>
    struct HasForcedValueOrThrow008 : std::false_type
    {
    };
    template <typename R>
    struct HasForcedValueOrThrow008<R, std::void_t<decltype(std::declval<R>().template ValueOrThrow<ErrorCode>())>>
        : std::true_type
    {
    };
    static_assert(!HasForcedValueOrThrow008<Result<int, int>&>::value);
    static_assert(!HasForcedValueOrThrow008<const Result<int, int>&>::value);
    static_assert(!HasForcedValueOrThrow008<Result<int, int>&&>::value);
    static_assert(!HasForcedValueOrThrow008<Result<void, int>&>::value);
    static_assert(!HasForcedValueOrThrow008<const Result<void, int>&>::value);
    static_assert(!HasForcedValueOrThrow008<Result<void, int>&&>::value);
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    static_assert(!std::is_constructible_v<ErrorDomain, ErrorDomain::IdType, std::string_view>);
    static_assert(
      std::is_constructible_v<ErrorDomain, ErrorDomain::IdType, std::string_view, ErrorDomain::ExceptionConverter>);
#else
    static_assert(std::is_constructible_v<ErrorDomain, ErrorDomain::IdType, std::string_view>);
#endif

    static_assert(static_cast<int>(HasValueOrThrow008<Result<int>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow008<const Result<int>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow008<Result<int>&&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow008<const Result<void>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(std::is_same_v<decltype(std::declval<Result<int>&>().Value()), int&>);
    static_assert(std::is_same_v<decltype(std::declval<const Result<int>&>().Value()), const int&>);
    static_assert(std::is_same_v<decltype(std::declval<Result<int>&&>().Value()), int&&>);

    /* ----------------------------------------------------------------------------------- */
    /* Verify mutable and const access reference the same value.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ValueAccess, ReferencesStoredValue)
    {
      /* Arrange */
      auto r = Result<int>::FromValue(0);
      const auto& c = r;
      /* Act */
      auto& v = r.Value();
      v = 42;
      const auto& observed = c.Value();
      /* Expect */
      EXPECT_EQ(observed, 42);
      EXPECT_EQ(&v, &observed);
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify ownership can be moved out.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ValueAccess, MovesPayload)
    {
      /* Arrange */
      auto r = Result<std::unique_ptr<int>>::FromValue(std::make_unique<int>(42));
      /* Act */
      auto v = std::move(r).Value();
      /* Expect */
      ASSERT_NE(v, nullptr);
      EXPECT_EQ(*v, 42);
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify successful void access returns normally.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ValueAccess, VoidSuccess)
    {
      /* Arrange */
      const auto r = Result<void>::FromValue();
      /* Act */
      r.Value();
      /* Expect */
      EXPECT_TRUE(r.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify the internal storage guard terminates when no active payload exists.
     * 1. Arrange: Construct a valid storage object containing an integer value.
     * 2. Act: Prepare child-only probes that empty the slot before querying its index or payload.
     * 3. Expect: The child terminates instead of returning an invalid internal state.
     */
    TEST(AP_R3_CORE_008_ValueAccess, EmptyStorageGuardTerminates)
    {
      /* Arrange */
      detail::ResultStorage<int, ErrorCode> storage{std::in_place_index<c_resultIdx>, 42};

      /* Act */
      const auto access_index = [&storage]()
      {
        SlotsOf(storage)[0].reset();
        (void)storage.index();
      };
      const auto access_payload = [&storage]()
      {
        SlotsOf(storage)[0].reset();
        (void)storage.template get<c_resultIdx>();
      };

      /* Expect */
      EXPECT_DEATH(access_index(), "");
      EXPECT_DEATH(access_payload(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify the storage mismatch guard terminates on the wrong alternative.
     * 1. Arrange: Prepare value and error results whose private storage is probed with the opposite type.
     * 2. Act: Prepare child-only probes that retrieve the wrong alternative through each qualifier.
     * 3. Expect: The child process terminates instead of returning an invalid active alternative.
     */
    TEST(AP_R3_CORE_008_ValueAccess, WrongAlternativeTerminates)
    {
      /* Arrange */
      auto value_result = Result<int>::FromValue(42);
      auto error_result = Result<int>::FromError(MakeErrorCode(AdaptivePiErrc::kInvalidArgument));
      /* Act */
      const auto value_access = [&value_result]()
      {
        (void)StorageOf(value_result).template get<c_errorIdx>();
      };
      const auto value_const_access = [&value_result]()
      {
        (void)StorageOf(std::as_const(value_result)).template get<c_errorIdx>();
      };
      const auto value_rvalue_access = [&value_result]()
      {
        (void)std::move(StorageOf(value_result)).template get<c_errorIdx>();
      };
      const auto error_access = [&error_result]()
      {
        (void)StorageOf(error_result).template get<c_resultIdx>();
      };
      const auto error_const_access = [&error_result]()
      {
        (void)StorageOf(std::as_const(error_result)).template get<c_resultIdx>();
      };
      const auto error_rvalue_access = [&error_result]()
      {
        (void)std::move(StorageOf(error_result)).template get<c_resultIdx>();
      };

      /* Expect */
      EXPECT_DEATH(value_access(), "");
      EXPECT_DEATH(value_const_access(), "");
      EXPECT_DEATH(value_rvalue_access(), "");
      EXPECT_DEATH(error_access(), "");
      EXPECT_DEATH(error_const_access(), "");
      EXPECT_DEATH(error_rvalue_access(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify the private storage guards remain active for the const and rvalue accessors too.
     * 1. Arrange: Construct valid storage and a value result for the two failure scenarios.
     * 2. Act: Prepare child-only probes for empty slots and mismatched const/rvalue access.
     * 3. Expect: The child terminates instead of returning a value for an invalid internal state.
     */
    TEST(AP_R3_CORE_008_ValueAccess, StorageGuardConstAndRvalueTerminate)
    {
      /* Arrange */
      detail::ResultStorage<int, ErrorCode> storage{std::in_place_index<c_resultIdx>, 42};
      auto result = Result<int>::FromValue(42);

      /* Act */
      const auto empty_slot_const = [&storage]()
      {
        SlotsOf(storage)[0].reset();
        (void)std::as_const(storage).template get<c_resultIdx>();
      };
      const auto empty_slot_rvalue = [&storage]()
      {
        SlotsOf(storage)[0].reset();
        (void)std::move(storage).template get<c_resultIdx>();
      };
      const auto mismatched_const = [&result]()
      {
        (void)StorageOf(std::as_const(result)).template get<c_errorIdx>();
      };
      const auto mismatched_rvalue = [&result]()
      {
        (void)std::move(StorageOf(result)).template get<c_errorIdx>();
      };

      /* Expect */
      EXPECT_DEATH(empty_slot_const(), "");
      EXPECT_DEATH(empty_slot_rvalue(), "");
      EXPECT_DEATH(mismatched_const(), "");
      EXPECT_DEATH(mismatched_rvalue(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueMutable
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueMutable& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise Value through mutable access. */
    class AP_R3_CORE_008_ValueOnErrorTerminates : public ::testing::TestWithParam<Case008ValueMutable>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of Value.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Prepare invalid access for the child process.
     * 3. Expect: The invalid access terminates the child process.
     */
    TEST_P(AP_R3_CORE_008_ValueOnErrorTerminates, Terminates)
    {
      /* Arrange */
      auto r = Result<int>::FromError(MakeErrorCode(GetParam().error));
      /* Act */
      const auto access = [&r]()
      {
        (void)r.Value();
      };
      /* Expect */
      EXPECT_DEATH(access(), "");
    }
    std::string NameCase008ValueMutable(const ::testing::TestParamInfo<Case008ValueMutable>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(AllErrors, AP_R3_CORE_008_ValueOnErrorTerminates,
                             ::testing::Values(Case008ValueMutable{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                                               Case008ValueMutable{AdaptivePiErrc::kInvalidState, "InvalidState"},
                                               Case008ValueMutable{AdaptivePiErrc::kOperationFailed,
                                                                   "OperationFailed"}),
                             NameCase008ValueMutable);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueConst
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueConst& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise Value through const access. */
    class AP_R3_CORE_008_ValueOnErrorTerminates_Const : public ::testing::TestWithParam<Case008ValueConst>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of Value.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Prepare invalid access for the child process.
     * 3. Expect: The invalid access terminates the child process.
     */
    TEST_P(AP_R3_CORE_008_ValueOnErrorTerminates_Const, Terminates)
    {
      /* Arrange */
      auto r = Result<int>::FromError(MakeErrorCode(GetParam().error));
      /* Act */
      const auto access = [&r]()
      {
        (void)std::as_const(r).Value();
      };
      /* Expect */
      EXPECT_DEATH(access(), "");
    }
    std::string NameCase008ValueConst(const ::testing::TestParamInfo<Case008ValueConst>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(AllErrors, AP_R3_CORE_008_ValueOnErrorTerminates_Const,
                             ::testing::Values(Case008ValueConst{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                                               Case008ValueConst{AdaptivePiErrc::kInvalidState, "InvalidState"},
                                               Case008ValueConst{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
                             NameCase008ValueConst);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueRvalue
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueRvalue& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise Value through rvalue access. */
    class AP_R3_CORE_008_ValueOnErrorTerminates_Rvalue : public ::testing::TestWithParam<Case008ValueRvalue>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of Value.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Prepare invalid access for the child process.
     * 3. Expect: The invalid access terminates the child process.
     */
    TEST_P(AP_R3_CORE_008_ValueOnErrorTerminates_Rvalue, Terminates)
    {
      /* Arrange */
      auto r = Result<std::string>::FromError(MakeErrorCode(GetParam().error));
      /* Act */
      const auto access = [&r]()
      {
        (void)std::move(r).Value();
      };
      /* Expect */
      EXPECT_DEATH(access(), "");
    }
    std::string NameCase008ValueRvalue(const ::testing::TestParamInfo<Case008ValueRvalue>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(AllErrors, AP_R3_CORE_008_ValueOnErrorTerminates_Rvalue,
                             ::testing::Values(Case008ValueRvalue{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                                               Case008ValueRvalue{AdaptivePiErrc::kInvalidState, "InvalidState"},
                                               Case008ValueRvalue{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
                             NameCase008ValueRvalue);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueVoid
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueVoid& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise Value through void access. */
    class AP_R3_CORE_008_ValueOnErrorTerminates_Void : public ::testing::TestWithParam<Case008ValueVoid>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of Value.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Prepare invalid access for the child process.
     * 3. Expect: The invalid access terminates the child process.
     */
    TEST_P(AP_R3_CORE_008_ValueOnErrorTerminates_Void, Terminates)
    {
      /* Arrange */
      auto r = Result<void>::FromError(MakeErrorCode(GetParam().error));
      /* Act */
      const auto access = [&r]()
      {
        (void)r.Value();
      };
      /* Expect */
      EXPECT_DEATH(access(), "");
    }
    std::string NameCase008ValueVoid(const ::testing::TestParamInfo<Case008ValueVoid>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(AllErrors, AP_R3_CORE_008_ValueOnErrorTerminates_Void,
                             ::testing::Values(Case008ValueVoid{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                                               Case008ValueVoid{AdaptivePiErrc::kInvalidState, "InvalidState"},
                                               Case008ValueVoid{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
                             NameCase008ValueVoid);

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueOrThrowMutable
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueOrThrowMutable& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise ValueOrThrow through mutable access. */
    class AP_R3_CORE_008_ValueOrThrowConvertsDomainError : public ::testing::TestWithParam<Case008ValueOrThrowMutable>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of ValueOrThrow.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Invoke conversion and capture the domain exception.
     * 3. Expect: The exception preserves its original error and domain.
     */
    TEST_P(AP_R3_CORE_008_ValueOrThrowConvertsDomainError, ConvertsError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(GetParam().error);
      auto r = Result<int>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)r.ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed = exception.Error();
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the domain exception to preserve its ErrorCode";
      }
      const auto& observed_error = observed.value();
      EXPECT_EQ(observed_error, error);
      EXPECT_EQ(observed_error.Domain(), error.Domain());
    }
    std::string NameCase008ValueOrThrowMutable(const ::testing::TestParamInfo<Case008ValueOrThrowMutable>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(
      AllErrors, AP_R3_CORE_008_ValueOrThrowConvertsDomainError,
      ::testing::Values(Case008ValueOrThrowMutable{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                        Case008ValueOrThrowMutable{AdaptivePiErrc::kInvalidState, "InvalidState"},
                        Case008ValueOrThrowMutable{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
      NameCase008ValueOrThrowMutable);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueOrThrowConst
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueOrThrowConst& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise ValueOrThrow through const access. */
    class AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Const
        : public ::testing::TestWithParam<Case008ValueOrThrowConst>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of ValueOrThrow.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Invoke conversion and capture the domain exception.
     * 3. Expect: The exception preserves its original error and domain.
     */
    TEST_P(AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Const, ConvertsError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(GetParam().error);
      auto r = Result<int>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)std::as_const(r).ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed = exception.Error();
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the domain exception to preserve its ErrorCode";
      }
      const auto& observed_error = observed.value();
      EXPECT_EQ(observed_error, error);
      EXPECT_EQ(observed_error.Domain(), error.Domain());
    }
    std::string NameCase008ValueOrThrowConst(const ::testing::TestParamInfo<Case008ValueOrThrowConst>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(
      AllErrors, AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Const,
      ::testing::Values(Case008ValueOrThrowConst{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                        Case008ValueOrThrowConst{AdaptivePiErrc::kInvalidState, "InvalidState"},
                        Case008ValueOrThrowConst{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
      NameCase008ValueOrThrowConst);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueOrThrowRvalue
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueOrThrowRvalue& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise ValueOrThrow through rvalue access. */
    class AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Rvalue
        : public ::testing::TestWithParam<Case008ValueOrThrowRvalue>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of ValueOrThrow.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Invoke conversion and capture the domain exception.
     * 3. Expect: The exception preserves its original error and domain.
     */
    TEST_P(AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Rvalue, ConvertsError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(GetParam().error);
      auto r = Result<std::string>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)std::move(r).ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed = exception.Error();
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the domain exception to preserve its ErrorCode";
      }
      const auto& observed_error = observed.value();
      EXPECT_EQ(observed_error, error);
      EXPECT_EQ(observed_error.Domain(), error.Domain());
    }
    std::string NameCase008ValueOrThrowRvalue(const ::testing::TestParamInfo<Case008ValueOrThrowRvalue>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(
      AllErrors, AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Rvalue,
      ::testing::Values(Case008ValueOrThrowRvalue{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                        Case008ValueOrThrowRvalue{AdaptivePiErrc::kInvalidState, "InvalidState"},
                        Case008ValueOrThrowRvalue{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
      NameCase008ValueOrThrowRvalue);

    /* ----------------------------------------------------------------------------------- */

    struct Case008ValueOrThrowVoid
    {
        AdaptivePiErrc error;
        const char* name;
    };
    std::ostream& operator<<(std::ostream& stream, const Case008ValueOrThrowVoid& parameter)
    {
      return stream << parameter.name;
    }
    /* GetParam supplies each error to exercise ValueOrThrow through void access. */
    class AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Void : public ::testing::TestWithParam<Case008ValueOrThrowVoid>
    {
    };
    /* ----------------------------------------------------------------------------------- */
    /* Verify the required error behavior of ValueOrThrow.
     * 1. Arrange: Construct an error result using the selected domain error.
     * 2. Act: Invoke conversion and capture the domain exception.
     * 3. Expect: The exception preserves its original error and domain.
     */
    TEST_P(AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Void, ConvertsError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(GetParam().error);
      auto r = Result<void>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)r.ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed = exception.Error();
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the domain exception to preserve its ErrorCode";
      }
      const auto& observed_error = observed.value();
      EXPECT_EQ(observed_error, error);
      EXPECT_EQ(observed_error.Domain(), error.Domain());
    }
    std::string NameCase008ValueOrThrowVoid(const ::testing::TestParamInfo<Case008ValueOrThrowVoid>& info)
    {
      return info.param.name;
    }
    INSTANTIATE_TEST_SUITE_P(
      AllErrors, AP_R3_CORE_008_ValueOrThrowConvertsDomainError_Void,
      ::testing::Values(Case008ValueOrThrowVoid{AdaptivePiErrc::kInvalidArgument, "InvalidArgument"},
                        Case008ValueOrThrowVoid{AdaptivePiErrc::kInvalidState, "InvalidState"},
                        Case008ValueOrThrowVoid{AdaptivePiErrc::kOperationFailed, "OperationFailed"}),
      NameCase008ValueOrThrowVoid);

    /* ----------------------------------------------------------------------------------- */
    /* Verify all value-returning conversion overloads on success.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ThrowSuccess, PreservesReferences)
    {
      /* Arrange */
      auto r = Result<std::string>::FromValue("42");
      const auto& c = r;
      /* Act */
      auto& v = r.ValueOrThrow();
      const auto& cv = c.ValueOrThrow();
      auto&& mv = std::move(r).ValueOrThrow();
      /* Expect */
      EXPECT_EQ(v, "42");
      EXPECT_EQ(&v, &cv);
      EXPECT_EQ(&v, &mv);
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify conversion access supports move-only payloads.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ThrowSuccess, MovesPayload)
    {
      /* Arrange */
      auto r = Result<std::unique_ptr<int>>::FromValue(std::make_unique<int>(42));
      /* Act */
      auto v = std::move(r).ValueOrThrow();
      /* Expect */
      ASSERT_NE(v, nullptr);
      EXPECT_EQ(*v, 42);
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify successful void conversion returns normally.
     * 1. Arrange: Prepare the result and expected payload.
     * 2. Act: Exercise value access.
     * 3. Expect: Verify the payload and state.
     */
    TEST(AP_R3_CORE_008_ThrowSuccess, VoidSuccess)
    {
      /* Arrange */
      const auto r = Result<void>::FromValue();
      /* Act */
      r.ValueOrThrow();
      /* Expect */
      EXPECT_TRUE(r.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify exception-enabled domains reject a null converter.
     * 1. Arrange: Prepare a null conversion function.
     * 2. Act: Prepare domain construction for execution in the child process.
     * 3. Expect: Invalid domain construction terminates.
     */
    TEST(AP_R3_CORE_008_DomainConversion, NullConverterTerminates)
    {
      /* Arrange */
      const ErrorDomain::ExceptionConverter converter{nullptr};
      /* Act */
      const auto construct = [converter]()
      {
        const ErrorDomain domain{0x544553540008ULL, "NullConverter", converter};
        (void)domain;
      };
      /* Expect */
      EXPECT_DEATH(construct(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify a new domain selects its own exception without changes to Result.
     * 1. Arrange: Define a distinct exception, converter, and error result.
     * 2. Act: Convert the error through ValueOrThrow.
     * 3. Expect: The new domain exception preserves its code and originating domain.
     */
    TEST(AP_R3_CORE_008_DomainConversion, DispatchesCustomDomain)
    {
      /* Arrange */
      struct CustomDomainException
      {
          ErrorCode code;
      };
      const ErrorDomain domain{0x544553540009ULL, "CustomConversion", [](const ErrorCode& error)
                               {
                                 throw CustomDomainException{error};
                               }};
      const ErrorCode error{42, domain};
      const auto result = Result<int>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)result.ValueOrThrow();
      }
      catch (const CustomDomainException& exception)
      {
        observed = exception.code;
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the custom domain exception";
      }
      EXPECT_EQ(observed->Value(), 42);
      EXPECT_EQ(&observed->Domain(), &domain);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that a converter returning without throwing cannot report successful conversion.
     * 1. Arrange: Construct a domain with a converter that returns normally and an error from that domain.
     * 2. Act: Ask the domain to convert the error in a death-test child.
     * 3. Expect: The child terminates instead of returning to its caller.
     */
    TEST(AP_R3_CORE_008_DomainConversion, ReturningConverterTerminates)
    {
      /* Arrange */
      const ErrorDomain domain{0x54455354000AULL, "ReturningConverter", [](const ErrorCode&) {}};
      const ErrorCode error{42, domain};

      /* Act */
      const auto convert = [&domain, &error]()
      {
        domain.ThrowAsException(error);
      };

      /* Expect */
      EXPECT_DEATH(convert(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify the conversion guard also terminates when the stored converter is cleared.
     * 1. Arrange: Construct a valid domain, replace its converter with null, and prepare its error.
     * 2. Act: Ask the domain to convert the error in a child process.
     * 3. Expect: The child terminates instead of falling through a null conversion boundary.
     */
    TEST(AP_R3_CORE_008_DomainConversion, NullConverterThrowTerminates)
    {
      /* Arrange */
      ErrorDomain domain{0x54455354000BULL, "NullThrower", [](const ErrorCode&) {}};
      const ErrorCode error{42, domain};
      ExceptionConverterOf(domain) = nullptr;

      /* Act */
      const auto convert = [&domain, &error]()
      {
        domain.ThrowAsException(error);
      };

      /* Expect */
      EXPECT_DEATH(convert(), "");
    }

#endif
    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_008_MutableStorageGuardDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_008_MutableStorageGuardDeathTestTypes =
      ::testing::Types<std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::string, ErrorCode, std::integral_constant<std::size_t, 0>>>;
    struct AP_R3_CORE_008_MutableStorageGuardDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 3> names{"IntValue", "IntError", "StringValue"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_008_MutableStorageGuardDeathTest, AP_R3_CORE_008_MutableStorageGuardDeathTestTypes,
                     AP_R3_CORE_008_MutableStorageGuardDeathTestNames);

    /* Exercise the selected getter's success, empty-slot and wrong-alternative paths.
     * Coverage-only: empty slots are infeasible through the public API; direct
     * error-storage mismatch probes also bypass the public Error() state check.
     * 1. Arrange: Construct live value/error alternatives and select the requested one.
     * 2. Act: Retrieve the valid reference and prepare child-only storage mutations.
     * 3. Expect: Valid access aliases the payload; empty and mismatched access terminate. */
    TYPED_TEST(AP_R3_CORE_008_MutableStorageGuardDeathTest, ForcedStatesValidateStorageGuard)
    {
      /* Arrange */
      using Value = std::tuple_element_t<0, TypeParam>;
      using Error = std::tuple_element_t<1, TypeParam>;
      constexpr std::size_t index = std::tuple_element_t<2, TypeParam>::value;
      detail::ResultStorage<Value, Error> storage{std::in_place_index<c_resultIdx>};
      /* Prepare both concrete alternatives so the mismatch probe can switch to
       * a real object of the wrong type without type-punning or invalid lifetimes.
       */
      if constexpr (std::is_same_v<Error, ErrorCode>)
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>, MakeErrorCode(AdaptivePiErrc::kInvalidState));
      }
      else
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>);
      }
      ActiveIndexOf(storage) = index;
      const auto& selected_slot = SlotsOf(storage)[index];

      /* Act */
      const auto empty = [&storage]()
      {
        SlotsOf(storage)[ActiveIndexOf(storage)].reset();
        (void)storage.template get<index>();
      };
      const auto mismatched = [&storage]()
      {
        ActiveIndexOf(storage) = 1U - ActiveIndexOf(storage);
        (void)storage.template get<index>();
      };
      /* Binding this reference does not move the stored payload. */
      auto&& observed = storage.template get<index>();

      /* Expect */
      if (!selected_slot.has_value())
      {
        ADD_FAILURE() << "The parent must retain its constructed payload";
        return;
      }
      EXPECT_EQ(std::addressof(observed), std::addressof(std::get<index>(selected_slot.value())));
      EXPECT_DEATH(empty(), "");
      EXPECT_DEATH(mismatched(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_008_ConstStorageGuardDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_008_ConstStorageGuardDeathTestTypes =
      ::testing::Types<std::tuple<long, int, std::integral_constant<std::size_t, 1>>,
                       std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::string, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::monostate, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::string, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<std::unique_ptr<int>, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<int, std::unique_ptr<int>, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::monostate, std::unique_ptr<int>, std::integral_constant<std::size_t, 1>>>;
    struct AP_R3_CORE_008_ConstStorageGuardDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 9> names{"LongIntError",       "IntValue",      "IntError",
                                                     "StringError",        "VoidError",     "StringValue",
                                                     "MoveOnlyValueError", "MoveOnlyError", "VoidMoveOnlyError"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_008_ConstStorageGuardDeathTest, AP_R3_CORE_008_ConstStorageGuardDeathTestTypes,
                     AP_R3_CORE_008_ConstStorageGuardDeathTestNames);

    /* Exercise the selected getter's success, empty-slot and wrong-alternative paths.
     * Coverage-only: empty slots are infeasible through the public API; direct
     * error-storage mismatch probes also bypass the public Error() state check.
     * 1. Arrange: Construct live value/error alternatives and select the requested one.
     * 2. Act: Retrieve the valid reference and prepare child-only storage mutations.
     * 3. Expect: Valid access aliases the payload; empty and mismatched access terminate. */
    TYPED_TEST(AP_R3_CORE_008_ConstStorageGuardDeathTest, ForcedStatesValidateStorageGuard)
    {
      /* Arrange */
      using Value = std::tuple_element_t<0, TypeParam>;
      using Error = std::tuple_element_t<1, TypeParam>;
      constexpr std::size_t index = std::tuple_element_t<2, TypeParam>::value;
      detail::ResultStorage<Value, Error> storage{std::in_place_index<c_resultIdx>};
      /* Prepare both concrete alternatives so the mismatch probe can switch to
       * a real object of the wrong type without type-punning or invalid lifetimes.
       */
      if constexpr (std::is_same_v<Error, ErrorCode>)
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>, MakeErrorCode(AdaptivePiErrc::kInvalidState));
      }
      else
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>);
      }
      ActiveIndexOf(storage) = index;
      const auto& selected_slot = SlotsOf(storage)[index];

      /* Act */
      const auto empty = [&storage]()
      {
        SlotsOf(storage)[ActiveIndexOf(storage)].reset();
        (void)std::as_const(storage).template get<index>();
      };
      const auto mismatched = [&storage]()
      {
        ActiveIndexOf(storage) = 1U - ActiveIndexOf(storage);
        (void)std::as_const(storage).template get<index>();
      };
      /* Binding this reference does not move the stored payload. */
      auto&& observed = std::as_const(storage).template get<index>();

      /* Expect */
      if (!selected_slot.has_value())
      {
        ADD_FAILURE() << "The parent must retain its constructed payload";
        return;
      }
      EXPECT_EQ(std::addressof(observed), std::addressof(std::get<index>(selected_slot.value())));
      EXPECT_DEATH(empty(), "");
      EXPECT_DEATH(mismatched(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_008_RvalueStorageGuardDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_008_RvalueStorageGuardDeathTestTypes =
      ::testing::Types<std::tuple<std::unique_ptr<int>, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<int, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<std::string, ErrorCode, std::integral_constant<std::size_t, 0>>,
                       std::tuple<std::monostate, ErrorCode, std::integral_constant<std::size_t, 1>>,
                       std::tuple<int, std::unique_ptr<int>, std::integral_constant<std::size_t, 1>>,
                       std::tuple<std::monostate, std::unique_ptr<int>, std::integral_constant<std::size_t, 1>>>;
    struct AP_R3_CORE_008_RvalueStorageGuardDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 7> names{
            "MoveOnlyValue", "IntError", "IntValue", "StringValue", "VoidError", "MoveOnlyError", "VoidMoveOnlyError"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_008_RvalueStorageGuardDeathTest, AP_R3_CORE_008_RvalueStorageGuardDeathTestTypes,
                     AP_R3_CORE_008_RvalueStorageGuardDeathTestNames);

    /* Exercise the selected getter's success, empty-slot and wrong-alternative paths.
     * Coverage-only: empty slots are infeasible through the public API; direct
     * error-storage mismatch probes also bypass the public Error() state check.
     * 1. Arrange: Construct live value/error alternatives and select the requested one.
     * 2. Act: Retrieve the valid reference and prepare child-only storage mutations.
     * 3. Expect: Valid access aliases the payload; empty and mismatched access terminate. */
    TYPED_TEST(AP_R3_CORE_008_RvalueStorageGuardDeathTest, ForcedStatesValidateStorageGuard)
    {
      /* Arrange */
      using Value = std::tuple_element_t<0, TypeParam>;
      using Error = std::tuple_element_t<1, TypeParam>;
      constexpr std::size_t index = std::tuple_element_t<2, TypeParam>::value;
      detail::ResultStorage<Value, Error> storage{std::in_place_index<c_resultIdx>};
      /* Prepare both concrete alternatives so the mismatch probe can switch to
       * a real object of the wrong type without type-punning or invalid lifetimes.
       */
      if constexpr (std::is_same_v<Error, ErrorCode>)
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>, MakeErrorCode(AdaptivePiErrc::kInvalidState));
      }
      else
      {
        SlotsOf(storage)[1].emplace(std::in_place_index<c_errorIdx>);
      }
      ActiveIndexOf(storage) = index;
      const auto& selected_slot = SlotsOf(storage)[index];

      /* Act */
      const auto empty = [&storage]()
      {
        SlotsOf(storage)[ActiveIndexOf(storage)].reset();
        (void)std::move(storage).template get<index>();
      };
      const auto mismatched = [&storage]()
      {
        ActiveIndexOf(storage) = 1U - ActiveIndexOf(storage);
        (void)std::move(storage).template get<index>();
      };
      /* Binding this reference does not move the stored payload. */
      auto&& observed = std::move(storage).template get<index>();

      /* Expect */
      if (!selected_slot.has_value())
      {
        ADD_FAILURE() << "The parent must retain its constructed payload";
        return;
      }
      EXPECT_EQ(std::addressof(observed), std::addressof(std::get<index>(selected_slot.value())));
      EXPECT_DEATH(empty(), "");
      EXPECT_DEATH(mismatched(), "");
    }

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_008_PayloadConversion : public ::testing::Test
    {
    };
    using AP_R3_CORE_008_PayloadConversionTypes = ::testing::Types<int, std::string, std::unique_ptr<int>>;
    struct AP_R3_CORE_008_PayloadConversionNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 3> names{"Int", "String", "MoveOnly"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_008_PayloadConversion, AP_R3_CORE_008_PayloadConversionTypes,
                     AP_R3_CORE_008_PayloadConversionNames);

    /* Verify both conversion outcomes for payload types previously tested on only one path.
     * 1. Arrange: Construct a success and an error result with the selected payload type.
     * 2. Act: Access the successful rvalue and catch conversion of the error rvalue.
     * 3. Expect: Successful access aliases the payload and conversion preserves the error. */
    TYPED_TEST(AP_R3_CORE_008_PayloadConversion, ConvertsErrorAndReturnsValue)
    {
      /* Arrange */
      auto success = Result<TypeParam>::FromValue();
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidState);
      auto failure = Result<TypeParam>::FromError(error);
      const auto& selected_slot = SlotsOf(StorageOf(success))[ActiveIndexOf(StorageOf(success))];
      std::optional<ErrorCode> observed_error;

      /* Act */
      auto&& observed_value = std::move(success).ValueOrThrow();
      try
      {
        (void)std::move(failure).ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed_error = exception.Error();
      }

      /* Expect */
      if (!selected_slot.has_value())
      {
        ADD_FAILURE() << "The parent must retain its constructed payload";
        return;
      }
      EXPECT_EQ(std::addressof(observed_value), std::addressof(std::get<c_resultIdx>(selected_slot.value())));
      ASSERT_TRUE(observed_error.has_value());
      EXPECT_EQ(*observed_error, error);
    }

#endif

    /* ============================= End Test_AP_R3_CORE_008 ============================= */

    /* =============================== Test_AP_R3_CORE_009 =============================== */

    /* ----------------------------------------------------------------------------------- */
    /* Verify error access returns the stored error only on error results.
     * 1. Arrange: Prepare a value result and an error result with the same domain error.
     * 2. Act: Read the stored error from the error result and attempt to read it from the value result.
     * 3. Expect: The error matches the original payload and access on a value result terminates.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, ReturnsStoredError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidArgument);
      auto value_result = Result<int>::FromValue(42);
      auto error_result = Result<int>::FromError(error);
      /* Act */
      const auto& observed = error_result.Error();
      const auto access = [&value_result]()
      {
        (void)value_result.Error();
      };
      /* Expect */
      EXPECT_EQ(observed, error);
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify the void result exposes the stored error without terminating.
     * 1. Arrange: Prepare a void result holding the selected failure code.
     * 2. Act: Read the stored error.
     * 3. Expect: The result remains in error state and returns the original code.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, VoidReturnsStoredError)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kOperationFailed);
      auto result = Result<void>::FromError(error);
      /* Act */
      const auto& observed = result.Error();
      /* Expect */
      EXPECT_EQ(observed, error);
      EXPECT_FALSE(result.HasValue());
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify error access on a successful result terminates the process.
     * 1. Arrange: Construct a success result for each selected failure state.
     * 2. Act: Attempt invalid error access.
     * 3. Expect: The process terminates.
     */
    TEST(AP_R3_CORE_009_ErrorOnValueTerminates, Terminates)
    {
      /* Arrange */
      auto result = Result<int>::FromValue(42);
      const auto access = [&result]()
      {
        (void)result.Error();
      };
      /* Expect */
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that rvalue error access on a successful non-void result terminates.
     * 1. Arrange: Construct a successful result with an integer value.
     * 2. Act: Call the rvalue Error() overload in a death-test child.
     * 3. Expect: The child terminates instead of returning an error.
     */
    TEST(AP_R3_CORE_009_ErrorOnValueTerminates, RvalueTerminates)
    {
      /* Arrange */
      auto result = Result<int>::FromValue(42);

      /* Act */
      const auto access = [&result]()
      {
        (void)std::move(result).Error();
      };

      /* Expect */
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify that rvalue error access on a successful void result terminates.
     * 1. Arrange: Construct a successful void result.
     * 2. Act: Call the rvalue Error() overload in a death-test child.
     * 3. Expect: The child terminates instead of returning an error.
     */
    TEST(AP_R3_CORE_009_ErrorOnValueTerminates, VoidRvalueTerminates)
    {
      /* Arrange */
      auto result = Result<void>::FromValue();

      /* Act */
      const auto access = [&result]()
      {
        (void)std::move(result).Error();
      };

      /* Expect */
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify void error access terminates when the result is successful.
     * 1. Arrange: Construct a successful void result in both const and rvalue forms.
     * 2. Act: Prepare child-only calls to the const and rvalue Error() overloads.
     * 3. Expect: Each child process terminates instead of returning an error from a value result.
     */
    TEST(AP_R3_CORE_009_ErrorOnValueTerminates, VoidResultErrorAccessTerminates)
    {
      /* Arrange */
      auto result = Result<void>::FromValue();
      /* Act */
      const auto const_access = [&result]()
      {
        (void)std::as_const(result).Error();
      };
      const auto rvalue_access = [&result]()
      {
        (void)std::move(result).Error();
      };

      /* Expect */
      EXPECT_DEATH(const_access(), "");
      EXPECT_DEATH(rvalue_access(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify rvalue error access transfers a move-only payload.
     * 1. Arrange: Construct an error result owning an integer and save its address.
     * 2. Act: Extract ownership through rvalue Error().
     * 3. Expect: The extracted error retains its address and value.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, MovesErrorPayload)
    {
      /* Arrange */
      auto result = Result<int, std::unique_ptr<int>>::FromError(std::make_unique<int>(42));
      const auto* original = result.Error().get();

      /* Act */
      auto observed = std::move(result).Error();

      /* Expect */
      ASSERT_NE(observed, nullptr);
      EXPECT_EQ(observed.get(), original);
      EXPECT_EQ(*observed, 42);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify rvalue error access transfers a move-only payload.
     * 1. Arrange: Construct an error result owning an integer and save its address.
     * 2. Act: Extract ownership through rvalue Error().
     * 3. Expect: The extracted error retains its address and value.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, VoidMovesErrorPayload)
    {
      /* Arrange */
      auto result = Result<void, std::unique_ptr<int>>::FromError(std::make_unique<int>(42));
      const auto* original = result.Error().get();

      /* Act */
      auto observed = std::move(result).Error();

      /* Expect */
      ASSERT_NE(observed, nullptr);
      EXPECT_EQ(observed.get(), original);
      EXPECT_EQ(*observed, 42);
    }

    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_009_ConstPayloadErrorDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_009_ConstPayloadErrorDeathTestTypes =
      ::testing::Types<Result<long, int>, Result<int>, Result<int, std::unique_ptr<int>>, Result<std::string>,
                       Result<void>, Result<void, std::unique_ptr<int>>>;
    struct AP_R3_CORE_009_ConstPayloadErrorDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 6> names{"LongInt",          "IntDefaultError",
                                                     "MoveOnlyError",    "StringDefaultError",
                                                     "VoidDefaultError", "VoidMoveOnlyError"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_009_ConstPayloadErrorDeathTest, AP_R3_CORE_009_ConstPayloadErrorDeathTestTypes,
                     AP_R3_CORE_009_ConstPayloadErrorDeathTestNames);

    /* Verify Error() rejects success for every instantiated payload type.
     * This is a reachable public-API misuse case, not an infeasible storage state.
     * 1. Arrange: Construct a successful result of the selected type.
     * 2. Act: Prepare error access through the selected reference qualifier.
     * 3. Expect: Invalid error access terminates the child process. */
    TYPED_TEST(AP_R3_CORE_009_ConstPayloadErrorDeathTest, SuccessRejectsErrorAccess)
    {
      /* Arrange */
      auto result = TypeParam::FromValue();

      /* Act */
      const auto access = [&result]()
      {
        (void)std::as_const(result).Error();
      };

      /* Expect */
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Each TypeParam selects an existing template instantiation. No production
     * behavior depends on this test-only type list.
     */
    template <typename T>
    class AP_R3_CORE_009_RvaluePayloadErrorDeathTest : public ::testing::Test
    {
    };
    using AP_R3_CORE_009_RvaluePayloadErrorDeathTestTypes =
      ::testing::Types<Result<int>, Result<int, std::unique_ptr<int>>, Result<void>,
                       Result<void, std::unique_ptr<int>>>;
    struct AP_R3_CORE_009_RvaluePayloadErrorDeathTestNames
    {
        template <typename>
        static std::string GetName(int index)
        {
          constexpr std::array<const char*, 4> names{"IntDefaultError", "MoveOnlyError", "VoidDefaultError",
                                                     "VoidMoveOnlyError"};
          return names.at(static_cast<std::size_t>(index));
        }
    };
    TYPED_TEST_SUITE(AP_R3_CORE_009_RvaluePayloadErrorDeathTest, AP_R3_CORE_009_RvaluePayloadErrorDeathTestTypes,
                     AP_R3_CORE_009_RvaluePayloadErrorDeathTestNames);

    /* Verify Error() rejects success for every instantiated payload type.
     * This is a reachable public-API misuse case, not an infeasible storage state.
     * 1. Arrange: Construct a successful result of the selected type.
     * 2. Act: Prepare error access through the selected reference qualifier.
     * 3. Expect: Invalid error access terminates the child process. */
    TYPED_TEST(AP_R3_CORE_009_RvaluePayloadErrorDeathTest, SuccessRejectsErrorAccess)
    {
      /* Arrange */
      auto result = TypeParam::FromValue();

      /* Act */
      const auto access = [&result]()
      {
        (void)std::move(result).Error();
      };

      /* Expect */
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify successful rvalue Error() access preserves the exact stored ErrorCode.
     * 1. Arrange: Construct a failure result and capture the stored object's address.
     * 2. Act: Retrieve the error through the rvalue overload.
     * 3. Expect: The returned reference aliases the stored error and preserves its metadata.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, ReturnsDefaultErrorFromRvalue)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidState);
      auto result = Result<int>::FromError(error);
      const auto* expected = &result.Error();

      /* Act */
      auto&& observed = std::move(result).Error();

      /* Expect */
      EXPECT_EQ(&observed, expected);
      EXPECT_EQ(observed, error);
      EXPECT_EQ(&observed.Domain(), &error.Domain());
    }

    /* ----------------------------------------------------------------------------------- */

    /* Verify successful rvalue Error() access preserves the exact stored ErrorCode.
     * 1. Arrange: Construct a failure result and capture the stored object's address.
     * 2. Act: Retrieve the error through the rvalue overload.
     * 3. Expect: The returned reference aliases the stored error and preserves its metadata.
     */
    TEST(AP_R3_CORE_009_ErrorAccess, VoidReturnsDefaultErrorFromRvalue)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidState);
      auto result = Result<void>::FromError(error);
      const auto* expected = &result.Error();

      /* Act */
      auto&& observed = std::move(result).Error();

      /* Expect */
      EXPECT_EQ(&observed, expected);
      EXPECT_EQ(observed, error);
      EXPECT_EQ(&observed.Domain(), &error.Domain());
    }

    /* ============================= End Test_AP_R3_CORE_009 ============================= */

    /* =============================== Test_AP_R3_CORE_010 =============================== */

    /* ----------------------------------------------------------------------------------- */
    /* Verify moving a value result preserves the selected alternative and payload.
     * 1. Arrange: Construct one value result and one error result.
     * 2. Act: Move both results into new destinations.
     * 3. Expect: The moved destinations retain their original state and payload.
     */
    TEST(AP_R3_CORE_010_MovePreservesState, ValueResultKeepsState)
    {
      /* Arrange */
      auto value_result = Result<std::string>::FromValue("payload");
      auto error_result = Result<std::string>::FromError(MakeErrorCode(AdaptivePiErrc::kInvalidState));

      /* Act */
      const Result<std::string> moved_value{std::move(value_result)};
      const Result<std::string> moved_error{std::move(error_result)};

      /* Expect */
      EXPECT_TRUE(moved_value.HasValue());
      EXPECT_EQ(moved_value.Value(), "payload");
      EXPECT_FALSE(moved_error.HasValue());
      EXPECT_EQ(moved_error.Error(), MakeErrorCode(AdaptivePiErrc::kInvalidState));
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify moving a void result preserves the selected alternative and error payload.
     * 1. Arrange: Construct one successful void result and one error void result.
     * 2. Act: Move both results into new destinations.
     * 3. Expect: The moved destinations retain their original success/error state.
     */
    TEST(AP_R3_CORE_010_MovePreservesState, VoidResultKeepsState)
    {
      /* Arrange */
      auto value_result = Result<void>::FromValue();
      auto error_result = Result<void>::FromError(MakeErrorCode(AdaptivePiErrc::kOperationFailed));

      /* Act */
      const Result<void> moved_value{std::move(value_result)};
      const Result<void> moved_error{std::move(error_result)};

      /* Expect */
      EXPECT_TRUE(moved_value.HasValue());
      EXPECT_FALSE(moved_error.HasValue());
      EXPECT_EQ(moved_error.Error(), MakeErrorCode(AdaptivePiErrc::kOperationFailed));
    }

    /* ============================= End Test_AP_R3_CORE_010 ============================= */

    /* =============================== Test_AP_R3_CORE_011 =============================== */

    static_assert(std::is_constructible_v<Result<int>, ErrorCode>);
    static_assert(!std::is_convertible_v<ErrorCode, Result<int>>);
    static_assert(std::is_constructible_v<Result<void, ErrorCode>, ErrorCode>);
    static_assert(!std::is_convertible_v<ErrorCode, Result<void, ErrorCode>>);

    template <typename R, typename = void>
    struct HasValueOrThrow011 : std::false_type
    {
    };
    template <typename R>
    struct HasValueOrThrow011<R, std::void_t<decltype(std::declval<R>().ValueOrThrow())>> : std::true_type
    {
    };
    static_assert(static_cast<int>(HasValueOrThrow011<Result<int>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow011<const Result<int>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow011<Result<int>&&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);
    static_assert(static_cast<int>(HasValueOrThrow011<const Result<void>&>::value) == ADAPTIVE_PI_EXCEPTIONS_ENABLED);

    /* ----------------------------------------------------------------------------------- */
    /* Verify recoverable failures remain explicit Result states instead of direct throws.
     * 1. Arrange: Prepare one successful result and one error result.
     * 2. Act: Query the result states and error access for the failing case.
     * 3. Expect: The public failure path stays in Result form and invalid value access still terminates.
     */
    TEST(AP_R3_CORE_011_ExceptionFreeResultUse, PublicFailureUsesResultState)
    {
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kInvalidArgument);
      auto value_result = Result<int>::FromValue(42);
      auto error_result = Result<int>::FromError(error);
      /* Act */
      const bool value_has_value = value_result.HasValue();
      const bool error_has_value = error_result.HasValue();
      const auto& observed = error_result.Error();
      const auto value = value_result.Value();
      const auto access = [&value_result]()
      {
        (void)value_result.Error();
      };
      /* Expect */
      EXPECT_TRUE(value_has_value);
      EXPECT_FALSE(error_has_value);
      EXPECT_EQ(value, 42);
      EXPECT_EQ(observed, error);
      EXPECT_DEATH(access(), "");
    }

    /* ----------------------------------------------------------------------------------- */
    /* Verify exception conversion remains an opt-in boundary rather than the default API contract.
     * 1. Arrange: Prepare an error result and test the build-specific API presence.
     * 2. Act: Invoke the conversion boundary only when exceptions are enabled.
     * 3. Expect: Result-based failure handling stays available in all builds, while direct throw conversion remains
     * guarded.
     */
    TEST(AP_R3_CORE_011_ExceptionFreeResultUse, ConversionIsOptionalBoundary)
    {
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
      /* Arrange */
      const auto error = MakeErrorCode(AdaptivePiErrc::kOperationFailed);
      auto result = Result<int>::FromError(error);
      std::optional<ErrorCode> observed;
      /* Act */
      try
      {
        (void)result.ValueOrThrow();
      }
      catch (const AdaptivePiException& exception)
      {
        observed = exception.Error();
      }
      /* Expect */
      if (!observed.has_value())
      {
        FAIL() << "Expected the domain exception to preserve its ErrorCode";
      }
      const auto& observed_error = observed.value();
      EXPECT_EQ(observed_error, error);
#else
      /* Arrange */
      constexpr bool has_value_or_throw = HasValueOrThrow011<Result<int>&>::value;
      /* Expect */
      EXPECT_FALSE(has_value_or_throw);
#endif
    }

    /* ============================= End Test_AP_R3_CORE_011 ============================= */

    /* Concrete member allowlist for the existing coverage probes. These explicit
     * instantiations expose only the named members, without changing production
     * declarations or access specifiers.
     */
    template struct ExposeResultProbe<int, ErrorCode, &Result<int, ErrorCode>::storage_,
                                      &TestStorage<int, ErrorCode>::slots_, &TestStorage<int, ErrorCode>::active_>;

    template struct ExposeResultProbe<int, int, &Result<int, int>::storage_, &TestStorage<int, int>::slots_,
                                      &TestStorage<int, int>::active_>;

    template struct ExposeResultProbe<void, ErrorCode, &Result<void, ErrorCode>::storage_,
                                      &TestStorage<void, ErrorCode>::slots_, &TestStorage<void, ErrorCode>::active_>;

    template struct ExposeResultProbe<long, int, &Result<long, int>::storage_, &TestStorage<long, int>::slots_,
                                      &TestStorage<long, int>::active_>;

    template struct ExposeResultProbe<int, AdaptivePiErrc, &Result<int, AdaptivePiErrc>::storage_,
                                      &TestStorage<int, AdaptivePiErrc>::slots_,
                                      &TestStorage<int, AdaptivePiErrc>::active_>;

    template struct ExposeResultProbe<void, AdaptivePiErrc, &Result<void, AdaptivePiErrc>::storage_,
                                      &TestStorage<void, AdaptivePiErrc>::slots_,
                                      &TestStorage<void, AdaptivePiErrc>::active_>;

    template struct ExposeResultProbe<bool, ErrorCode, &Result<bool, ErrorCode>::storage_,
                                      &TestStorage<bool, ErrorCode>::slots_, &TestStorage<bool, ErrorCode>::active_>;

    template struct ExposeResultProbe<void, int, &Result<void, int>::storage_, &TestStorage<void, int>::slots_,
                                      &TestStorage<void, int>::active_>;

    template struct ExposeResultProbe<std::string, ErrorCode, &Result<std::string, ErrorCode>::storage_,
                                      &TestStorage<std::string, ErrorCode>::slots_,
                                      &TestStorage<std::string, ErrorCode>::active_>;

    template struct ExposeResultProbe<
      std::unique_ptr<int>, ErrorCode, &Result<std::unique_ptr<int>, ErrorCode>::storage_,
      &TestStorage<std::unique_ptr<int>, ErrorCode>::slots_, &TestStorage<std::unique_ptr<int>, ErrorCode>::active_>;

    template struct ExposeResultProbe<int, std::unique_ptr<int>, &Result<int, std::unique_ptr<int>>::storage_,
                                      &TestStorage<int, std::unique_ptr<int>>::slots_,
                                      &TestStorage<int, std::unique_ptr<int>>::active_>;

    template struct ExposeResultProbe<void, std::unique_ptr<int>, &Result<void, std::unique_ptr<int>>::storage_,
                                      &TestStorage<void, std::unique_ptr<int>>::slots_,
                                      &TestStorage<void, std::unique_ptr<int>>::active_>;

    template struct ExposeResultProbe<
      TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
      TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
      &Result<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
              TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::storage_,
      &TestStorage<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
                   TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::slots_,
      &TestStorage<TrackedObject_ConstructsAndDestroysOnlySelectedAlternative,
                   TrackedObject_ConstructsAndDestroysOnlySelectedAlternative>::active_>;

    template struct ExposeResultProbe<
      void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure,
      &Result<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::storage_,
      &TestStorage<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::slots_,
      &TestStorage<void, VoidStateTrackedObject_ConstructsAnErrorOnlyOnFailure>::active_>;

    template struct ExposeResultProbe<
      ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
      TrackedError_DirectConstructionSupportsMoveOnlyAlternatives,
      &Result<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
              TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>::storage_,
      &TestStorage<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
                   TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>::slots_,
      &TestStorage<ConstructionTrackedObject_DirectConstructionSupportsMoveOnlyAlternatives,
                   TrackedError_DirectConstructionSupportsMoveOnlyAlternatives>::active_>;

    template struct ExposeResultProbe<
      ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc,
      &Result<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::storage_,
      &TestStorage<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::slots_,
      &TestStorage<ResultConstructionValueCase_FactoryFunctionsSupportInPlaceConstruction, AdaptivePiErrc>::active_>;

    template struct ExposeResultProbe<
      ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
      TrackedError_EmplaceValueAndErrorReplaceActiveAlternative,
      &Result<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
              TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>::storage_,
      &TestStorage<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
                   TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>::slots_,
      &TestStorage<ConstructionTrackedObject_EmplaceValueAndErrorReplaceActiveAlternative,
                   TrackedError_EmplaceValueAndErrorReplaceActiveAlternative>::active_>;

    template struct ExposeResultProbe<
      ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
      ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
      &Result<ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
              ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess>::storage_,
      &TestStorage<ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
                   ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess>::slots_,
      &TestStorage<ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess,
                   ObservedPayload_PreservesPayloadOnFailureAndReplacesOnSuccess>::active_>;

    template struct ExposeResultProbe<
      void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess,
      &Result<void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess>::storage_,
      &TestStorage<void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess>::slots_,
      &TestStorage<void, VoidObservedPayload_PreservesStateOnFailureAndReplacesOnSuccess>::active_>;

  } /* namespace */
} /* namespace ara::core */
