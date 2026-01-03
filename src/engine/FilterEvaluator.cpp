// Copyright 2026, QLever EPIC 10 Phase 3B
// Implementation of versioned filter evaluators
// EPIC 13: Removed fake SIMD implementation

#include "engine/FilterEvaluator.h"

#include "engine/CallFixedSize.h"
#include "engine/sparqlExpressions/SparqlExpressionValueGetters.h"
#include "util/Algorithm.h"

// _____________________________________________________________________________
// ScalarFilterEvaluator implementation (V1)
// Extracted from Filter::computeFilterImpl (Filter.cpp lines 131-227)
// _____________________________________________________________________________

IdTable ScalarFilterEvaluator::evaluate(
    const IdTable& input,
    const sparqlExpression::SparqlExpressionPimpl& expression,
    sparqlExpression::EvaluationContext& context) const {
  size_t width = input.numColumns();
  IdTable result{width, context._allocator};

  auto impl = [&expression, &result, &input, &context](auto WIDTH) {
    LocalVocab dummyLocalVocab{};
    IdTableStatic<WIDTH> resultTable =
        std::move(result).toStatic<static_cast<size_t>(WIDTH)>();

    // Evaluate expression on input table
    sparqlExpression::ExpressionResult expressionResult =
        expression.getPimpl()->evaluate(&context);

    // Filter input by expressionResult and store in resultTable
    auto computeResult =
        CPP_template_lambda(&resultTable = resultTable,
                            &input =
                                input
                                    .asStaticView<static_cast<size_t>(WIDTH)>(),
                            &context)(typename T)(T && singleResult)(
            requires sparqlExpression::SingleExpressionResult<T>) {
      if constexpr (std::is_same_v<T, ad_utility::SetOfIntervals>) {
        // Binary filter case: copy intervals from input to result
        auto totalSize = std::accumulate(
            singleResult._intervals.begin(), singleResult._intervals.end(),
            resultTable.size(),
            [&input](const auto& sum, const auto& interval) {
              size_t intervalBegin = interval.first;
              size_t intervalEnd = std::min(interval.second, input.size());
              return sum + (intervalEnd - intervalBegin);
            });

        if (resultTable.empty() && totalSize == input.size()) {
          // All elements pass filter, no need to copy
          // Note: This path requires special handling in caller
          return;
        }

        for (auto [intervalBegin, intervalEnd] : singleResult._intervals) {
          intervalEnd = std::min(intervalEnd, input.size());
          resultTable.insertAtEnd(input, intervalBegin, intervalEnd);
        }
      } else {
        // General case: evaluate expression per row
        auto resultGenerator = sparqlExpression::detail::makeGenerator(
            AD_FWD(singleResult), input.size(), &context);
        size_t i = 0;

        using ValueGetter =
            sparqlExpression::detail::EffectiveBooleanValueGetter;
        ValueGetter valueGetter{};
        for (auto&& resultValue : resultGenerator) {
          if (valueGetter(resultValue, &context) == ValueGetter::Result::True) {
            resultTable.push_back(input[i]);
          }
          ++i;
        }
      }
    };
    std::visit(computeResult, std::move(expressionResult));

    result = std::move(resultTable).toDynamic();
  };

  ad_utility::callFixedSizeVi(width, impl);
  return result;
}

// _____________________________________________________________________________
// AdaptiveFilterEvaluator implementation (V2)
// Currently always uses scalar; reserved for future optimizations
// _____________________________________________________________________________

AdaptiveFilterEvaluator::AdaptiveFilterEvaluator() {
  // Currently always use scalar evaluator
  // Future implementations may detect CPU capabilities and select optimized
  // implementations
  impl_ = std::make_unique<ScalarFilterEvaluator>();
}

// _____________________________________________________________________________
// FilterEvaluatorFactory implementation
// _____________________________________________________________________________

std::unique_ptr<FilterEvaluator> FilterEvaluatorFactory::create(
    EvaluatorType type) {
  switch (type) {
    case EvaluatorType::SCALAR:
      return std::make_unique<ScalarFilterEvaluator>();
    case EvaluatorType::ADAPTIVE:
    default:
      return std::make_unique<AdaptiveFilterEvaluator>();
  }
}
