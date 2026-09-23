#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/result.h"

#include <gtest/gtest.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace ara::core
{
  namespace
  {
    /* ========================== Test_AP_R3_CORE_005 ==================================== */

    struct ResultStateCase
    {
        bool has_value;
        int value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultStateCase& parameter)
    {
      return output << parameter.description;
    }

    // Non-default-constructible and move-only: the inactive alternative must not
    // require construction. Count live objects, independently of copy elision.
    class TrackedObject final
    {
      public:
        explicit TrackedObject(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        TrackedObject(const TrackedObject&) = delete;
        TrackedObject(TrackedObject&& other) noexcept : TrackedObject{other.live_count_} {}
        TrackedObject& operator=(const TrackedObject&) = delete;
        TrackedObject& operator=(TrackedObject&&) = delete;

        ~TrackedObject()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    class TrackedError final
    {
      public:
        explicit TrackedError(int& live_count) : live_count_{live_count}
        {
          ++live_count_;
        }

        TrackedError(const TrackedError&) = delete;
        TrackedError(TrackedError&& other) noexcept : TrackedError{other.live_count_} {}
        TrackedError& operator=(const TrackedError&) = delete;
        TrackedError& operator=(TrackedError&&) = delete;

        ~TrackedError()
        {
          --live_count_;
        }

      private:
        int& live_count_;
    };

    class AP_R3_CORE_005_ResultHasExactlyOneState : public ::testing::TestWithParam<ResultStateCase>
    {
    };

    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, ReportsSelectedState)
    {
      const ResultStateCase& parameter{GetParam()};
      const auto result = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                              : Result<int>::FromError(MakeErrorCode(parameter.error));

      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, CopyConstructionPreservesState)
    {
      const ResultStateCase& parameter{GetParam()};
      const auto source = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                              : Result<int>::FromError(MakeErrorCode(parameter.error));
      const Result<int> destination{source};

      EXPECT_EQ(destination.HasValue(), parameter.has_value);
      EXPECT_EQ(source.HasValue(), parameter.has_value);
    }

    // Also exercises AP-R3-CORE-010. Do not observe the moved-from result.
    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, MoveConstructionPreservesState)
    {
      const ResultStateCase& parameter{GetParam()};
      auto source = parameter.has_value ? Result<int>::FromValue(parameter.value)
                                        : Result<int>::FromError(MakeErrorCode(parameter.error));
      // Explicitly exercise move construction even when the payload is trivial.
      // NOLINTNEXTLINE(performance-move-const-arg)
      const Result<int> destination{std::move(source)};

      EXPECT_EQ(destination.HasValue(), parameter.has_value);
    }

    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, SameTypesAndPayloadsKeepAlternativesDistinct)
    {
      const ResultStateCase& parameter{GetParam()};
      const auto result = parameter.has_value ? Result<int, int>::FromValue(parameter.value)
                                              : Result<int, int>::FromError(parameter.value);

      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    TEST_P(AP_R3_CORE_005_ResultHasExactlyOneState, ConstructsAndDestroysOnlySelectedAlternative)
    {
      const ResultStateCase& parameter{GetParam()};
      int live_values{0};
      int live_errors{0};

      {
        const auto result = parameter.has_value
                              ? Result<TrackedObject, TrackedObject>::FromValue(TrackedObject{live_values})
                              : Result<TrackedObject, TrackedObject>::FromError(TrackedObject{live_errors});

        EXPECT_EQ(result.HasValue(), parameter.has_value);
        EXPECT_EQ(live_values, parameter.has_value ? 1 : 0);
        EXPECT_EQ(live_errors, parameter.has_value ? 0 : 1);
      }

      EXPECT_EQ(live_values, 0);
      EXPECT_EQ(live_errors, 0);
    }

    std::string ResultStateCaseName(const ::testing::TestParamInfo<ResultStateCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultStates, AP_R3_CORE_005_ResultHasExactlyOneState,
      ::testing::Values(
        ResultStateCase{true, 0, AdaptivePiErrc::kInvalidArgument, "ZeroValue", "Zero is a successful value"},
        ResultStateCase{true, 42, AdaptivePiErrc::kInvalidState, "PositiveValue", "Positive integer success"},
        ResultStateCase{true, -1, AdaptivePiErrc::kOperationFailed, "NegativeValue", "Negative integer success"},
        ResultStateCase{false, 0, AdaptivePiErrc::kInvalidArgument, "InvalidArgument", "Invalid argument failure"},
        ResultStateCase{false, 42, AdaptivePiErrc::kInvalidState, "InvalidState", "Invalid state failure"},
        ResultStateCase{false, -1, AdaptivePiErrc::kOperationFailed, "OperationFailed", "Operation failed"}),
      ResultStateCaseName);

    struct ResultVoidStateCase
    {
        bool has_value;
        AdaptivePiErrc error;
        std::string_view test_name;
        const char* description;
    };

    std::ostream& operator<<(std::ostream& output, const ResultVoidStateCase& parameter)
    {
      return output << parameter.description;
    }

    class AP_R3_CORE_005_ResultVoidHasExactlyOneState : public ::testing::TestWithParam<ResultVoidStateCase>
    {
    };

    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState, ReportsSelectedState)
    {
      const ResultVoidStateCase& parameter{GetParam()};
      const auto result =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));

      EXPECT_EQ(result.HasValue(), parameter.has_value);
    }

    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState, CopyConstructionPreservesState)
    {
      const ResultVoidStateCase& parameter{GetParam()};
      const auto source =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));
      const Result<void> destination{source};

      EXPECT_EQ(destination.HasValue(), parameter.has_value);
      EXPECT_EQ(source.HasValue(), parameter.has_value);
    }

    // Also exercises AP-R3-CORE-010. Do not observe the moved-from result.
    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState, MoveConstructionPreservesState)
    {
      const ResultVoidStateCase& parameter{GetParam()};
      auto source =
        parameter.has_value ? Result<void>::FromValue() : Result<void>::FromError(MakeErrorCode(parameter.error));
      // Explicitly exercise move construction even when the payload is trivial.
      // NOLINTNEXTLINE(performance-move-const-arg)
      const Result<void> destination{std::move(source)};

      EXPECT_EQ(destination.HasValue(), parameter.has_value);
    }

    TEST_P(AP_R3_CORE_005_ResultVoidHasExactlyOneState, ConstructsAnErrorOnlyOnFailure)
    {
      const ResultVoidStateCase& parameter{GetParam()};
      int live_errors{0};

      {
        const auto result = parameter.has_value ? Result<void, TrackedObject>::FromValue()
                                                : Result<void, TrackedObject>::FromError(TrackedObject{live_errors});

        EXPECT_EQ(result.HasValue(), parameter.has_value);
        EXPECT_EQ(live_errors, parameter.has_value ? 0 : 1);
      }

      EXPECT_EQ(live_errors, 0);
    }

    std::string ResultVoidStateCaseName(const ::testing::TestParamInfo<ResultVoidStateCase>& information)
    {
      return std::string{information.param.test_name};
    }

    INSTANTIATE_TEST_SUITE_P(
      ResultVoidStates, AP_R3_CORE_005_ResultVoidHasExactlyOneState,
      ::testing::Values(
        ResultVoidStateCase{true, AdaptivePiErrc::kInvalidArgument, "Success", "Successful completion without a value"},
        ResultVoidStateCase{false, AdaptivePiErrc::kInvalidArgument, "InvalidArgument", "Invalid argument failure"},
        ResultVoidStateCase{false, AdaptivePiErrc::kInvalidState, "InvalidState", "Invalid state failure"},
        ResultVoidStateCase{false, AdaptivePiErrc::kOperationFailed, "OperationFailed", "Operation failed"}),
      ResultVoidStateCaseName);

    /* ========================== Test_AP_R3_CORE_005 ==================================== */
    /* ========================== Test_AP_R3_CORE_006 ==================================== */

    struct ObservedPayload;

    struct PayloadObserver
    {
        const ObservedPayload* current{nullptr};
        int live{0};
    };

    // Observe contents and lifetime without exposing Result storage. Disallowing
    // copies and moves verifies genuine in-place construction.
    struct ObservedPayload
    {
        PayloadObserver& observer;
        std::string text;
        int number;

        ObservedPayload(PayloadObserver& observer_in, std::string text_in, int number_in, bool fail)
            : observer{observer_in}, text{std::move(text_in)}, number{number_in}
        {
          if (fail)
          {
            throw std::runtime_error{"payload construction failed"};
          }
          observer.current = this;
          ++observer.live;
        }

        ObservedPayload(const ObservedPayload&) = delete;
        ObservedPayload(ObservedPayload&&) = delete;
        ObservedPayload& operator=(const ObservedPayload&) = delete;
        ObservedPayload& operator=(ObservedPayload&&) = delete;

        ~ObservedPayload() noexcept
        {
          if (observer.current == this)
          {
            observer.current = nullptr;
          }
          --observer.live;
        }
    };

    struct ResultConstructionValueCase
    {
        std::string text;
        int value;

        ResultConstructionValueCase(std::string text_in, int value_in) : text{std::move(text_in)}, value{value_in} {}
    };

    class AP_R3_CORE_006_ResultCreationAndEmplacement : public ::testing::Test
    {
    };

    using ConstructorResult = Result<int, AdaptivePiErrc>;

    // Values may convert implicitly; errors require explicit construction.
    static_assert(std::is_convertible_v<int, ConstructorResult>);
    static_assert(std::is_constructible_v<ConstructorResult, const int&>);
    static_assert(std::is_constructible_v<ConstructorResult, const AdaptivePiErrc&>);
    static_assert(std::is_constructible_v<ConstructorResult, AdaptivePiErrc&&>);
    static_assert(!std::is_convertible_v<AdaptivePiErrc, ConstructorResult>);

    static_assert(std::is_constructible_v<Result<void, AdaptivePiErrc>, AdaptivePiErrc>);
    static_assert(!std::is_convertible_v<AdaptivePiErrc, Result<void, AdaptivePiErrc>>);

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, DirectConstructionSelectsValueOrError)
    {
      const int value{42};
      const AdaptivePiErrc error{AdaptivePiErrc::kInvalidArgument};

      const ConstructorResult copied_value{value};
      const ConstructorResult moved_value{42};
      const ConstructorResult copied_error{error};
      const ConstructorResult moved_error{AdaptivePiErrc::kOperationFailed};

      EXPECT_TRUE(copied_value.HasValue());
      EXPECT_TRUE(moved_value.HasValue());
      EXPECT_FALSE(copied_error.HasValue());
      EXPECT_FALSE(moved_error.HasValue());
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, DirectConstructionSupportsMoveOnlyAlternatives)
    {
      int live_values{0};
      int live_errors{0};

      {
        const Result<TrackedObject, TrackedError> value_result{TrackedObject{live_values}};
        const Result<TrackedObject, TrackedError> error_result{TrackedError{live_errors}};

        EXPECT_TRUE(value_result.HasValue());
        EXPECT_FALSE(error_result.HasValue());
        EXPECT_EQ(live_values, 1);
        EXPECT_EQ(live_errors, 1);
      }

      EXPECT_EQ(live_values, 0);
      EXPECT_EQ(live_errors, 0);
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, SameTypesUseDirectConstructionForValue)
    {
      const Result<int, int> value{42};
      const auto error = Result<int, int>::FromError(42);

      EXPECT_TRUE(value.HasValue());
      EXPECT_FALSE(error.HasValue());
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, VoidResultSupportsDirectErrorConstruction)
    {
      const AdaptivePiErrc error{AdaptivePiErrc::kInvalidArgument};

      const Result<void, AdaptivePiErrc> copied_error{error};
      const Result<void, AdaptivePiErrc> moved_error{AdaptivePiErrc::kOperationFailed};

      EXPECT_FALSE(copied_error.HasValue());
      EXPECT_FALSE(moved_error.HasValue());
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, FactoryFunctionsSupportInPlaceConstruction)
    {
      Result<ResultConstructionValueCase, AdaptivePiErrc> value_result =
        Result<ResultConstructionValueCase, AdaptivePiErrc>::FromValue("created-value", 42);
      EXPECT_TRUE(value_result.HasValue());

      Result<ResultConstructionValueCase, AdaptivePiErrc> error_result =
        Result<ResultConstructionValueCase, AdaptivePiErrc>::FromError(AdaptivePiErrc::kInvalidArgument);
      EXPECT_FALSE(error_result.HasValue());
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, EmplaceValueAndErrorReplaceActiveAlternative)
    {
      int live_values{0};
      int live_errors{0};

      Result<TrackedObject, TrackedError> result =
        Result<TrackedObject, TrackedError>::FromValue(TrackedObject{live_values});
      EXPECT_TRUE(result.HasValue());

      result.EmplaceError(live_errors);
      EXPECT_FALSE(result.HasValue());
      EXPECT_EQ(live_values, 0);
      EXPECT_EQ(live_errors, 1);

      result.EmplaceValue(live_values);
      EXPECT_TRUE(result.HasValue());
      EXPECT_EQ(live_values, 1);
      EXPECT_EQ(live_errors, 0);
    }

    TEST_F(AP_R3_CORE_006_ResultCreationAndEmplacement, VoidResultSupportsEmplacement)
    {
      Result<void, AdaptivePiErrc> result = Result<void, AdaptivePiErrc>::FromValue();
      EXPECT_TRUE(result.HasValue());

      result.EmplaceError(AdaptivePiErrc::kOperationFailed);
      EXPECT_FALSE(result.HasValue());

      result.EmplaceValue();
      EXPECT_TRUE(result.HasValue());
    }

    class AP_R3_CORE_006_ReplacementTransitions : public ::testing::TestWithParam<std::tuple<bool, bool, bool>>
    {
    };

    // All initial/replacement states, with and without an initial failed attempt.
    // Failed replacement must preserve CORE-005; our chosen policy also retains
    // the original payload and allows a subsequent retry.
    TEST_P(AP_R3_CORE_006_ReplacementTransitions, PreservesPayloadOnFailureAndReplacesOnSuccess)
    {
      const auto [starts_with_value, replaces_with_value, fail_first] = GetParam();
      using ObservedResult = Result<ObservedPayload, ObservedPayload>;
      PayloadObserver original;
      PayloadObserver replacement;
      {
        auto result = starts_with_value ? ObservedResult::FromValue(original, "original", 17, false)
                                        : ObservedResult::FromError(original, "original", 17, false);
        EXPECT_EQ(result.HasValue(), starts_with_value);
        ASSERT_NE(original.current, nullptr);
        EXPECT_EQ(original.current->text, "original");
        EXPECT_EQ(original.current->number, 17);
        EXPECT_EQ(original.live, 1);
        const ObservedPayload* original_address{original.current};

        if (fail_first)
        {
          if (replaces_with_value)
          {
            EXPECT_THROW(result.EmplaceValue(replacement, "replacement", 42, true), std::runtime_error);
          }
          else
          {
            EXPECT_THROW(result.EmplaceError(replacement, "replacement", 42, true), std::runtime_error);
          }
          EXPECT_EQ(result.HasValue(), starts_with_value);
          EXPECT_EQ(original.current, original_address);
          EXPECT_EQ(original.live, 1);
          EXPECT_EQ(replacement.current, nullptr);
          EXPECT_EQ(replacement.live, 0);
          // Guard dereferences when testing an implementation that destroys the old object.
          ASSERT_NE(original.current, nullptr);
          EXPECT_EQ(original.current->text, "original");
          EXPECT_EQ(original.current->number, 17);
        }

        if (replaces_with_value)
        {
          result.EmplaceValue(replacement, "replacement", 42, false);
        }
        else
        {
          result.EmplaceError(replacement, "replacement", 42, false);
        }
        EXPECT_EQ(result.HasValue(), replaces_with_value);
        EXPECT_EQ(original.current, nullptr);
        EXPECT_EQ(original.live, 0);
        ASSERT_NE(replacement.current, nullptr);
        EXPECT_EQ(replacement.current->text, "replacement");
        EXPECT_EQ(replacement.current->number, 42);
        EXPECT_EQ(replacement.live, 1);
      }
      EXPECT_EQ(original.live, 0);
      EXPECT_EQ(replacement.live, 0);
      EXPECT_EQ(replacement.current, nullptr);
    }

    std::string ReplacementCaseName(const ::testing::TestParamInfo<std::tuple<bool, bool, bool>>& information)
    {
      const auto [starts_with_value, replaces_with_value, fail_first] = information.param;
      return std::string{starts_with_value ? "ValueTo" : "ErrorTo"} + (replaces_with_value ? "Value" : "Error") +
             (fail_first ? "AfterFailure" : "Success");
    }

    INSTANTIATE_TEST_SUITE_P(AllStates, AP_R3_CORE_006_ReplacementTransitions,
                             ::testing::Combine(::testing::Bool(), ::testing::Bool(), ::testing::Bool()),
                             ReplacementCaseName);

    class AP_R3_CORE_006_VoidReplacement : public ::testing::TestWithParam<std::tuple<bool, bool>>
    {
    };

    TEST_P(AP_R3_CORE_006_VoidReplacement, PreservesStateOnFailureAndReplacesOnSuccess)
    {
      const auto [starts_with_value, fail_first] = GetParam();
      using ObservedResult = Result<void, ObservedPayload>;
      PayloadObserver original;
      PayloadObserver replacement;
      {
        auto result =
          starts_with_value ? ObservedResult::FromValue() : ObservedResult::FromError(original, "original", 17, false);
        EXPECT_EQ(result.HasValue(), starts_with_value);
        if (!starts_with_value)
        {
          ASSERT_NE(original.current, nullptr);
          EXPECT_EQ(original.current->text, "original");
          EXPECT_EQ(original.current->number, 17);
        }
        const ObservedPayload* original_address{original.current};
        if (fail_first)
        {
          EXPECT_THROW(result.EmplaceError(replacement, "replacement", 42, true), std::runtime_error);
          EXPECT_EQ(result.HasValue(), starts_with_value);
          EXPECT_EQ(original.current, original_address);
          EXPECT_EQ(original.live, starts_with_value ? 0 : 1);
          EXPECT_EQ(replacement.live, 0);
          EXPECT_EQ(replacement.current, nullptr);
          if (!starts_with_value)
          {
            ASSERT_NE(original.current, nullptr);
            EXPECT_EQ(original.current->text, "original");
            EXPECT_EQ(original.current->number, 17);
          }
        }

        result.EmplaceError(replacement, "replacement", 42, false);
        EXPECT_FALSE(result.HasValue());
        EXPECT_EQ(original.live, 0);
        EXPECT_EQ(original.current, nullptr);
        ASSERT_NE(replacement.current, nullptr);
        EXPECT_EQ(replacement.current->text, "replacement");
        EXPECT_EQ(replacement.current->number, 42);
        EXPECT_EQ(replacement.live, 1);

        result.EmplaceValue();
        EXPECT_TRUE(result.HasValue());
        EXPECT_EQ(replacement.live, 0);
        EXPECT_EQ(replacement.current, nullptr);
        result.EmplaceValue();
        EXPECT_TRUE(result.HasValue());
      }
      EXPECT_EQ(original.live, 0);
      EXPECT_EQ(replacement.live, 0);
    }

    std::string VoidReplacementCaseName(const ::testing::TestParamInfo<std::tuple<bool, bool>>& information)
    {
      const auto [starts_with_value, fail_first] = information.param;
      return std::string{starts_with_value ? "SuccessToError" : "ErrorToError"} +
             (fail_first ? "AfterFailure" : "Success");
    }

    INSTANTIATE_TEST_SUITE_P(AllStates, AP_R3_CORE_006_VoidReplacement,
                             ::testing::Combine(::testing::Bool(), ::testing::Bool()), VoidReplacementCaseName);

    /* ======================== End Test_AP_R3_CORE_006 ================================== */
  } // namespace
} // namespace ara::core
