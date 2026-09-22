#include "ara/core/adaptive_pi_error_domain.h"
#include "ara/core/result.h"

#include <gtest/gtest.h>
#include <ostream>
#include <string>
#include <string_view>
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

    /* ======================== End Test_AP_R3_CORE_005 ================================== */
  } // namespace
} // namespace ara::core
