// Copyright 2026, QLever EPIC 10 Phase 3B
// Implementation of versioned filter evaluators

#include "engine/FilterEvaluator.h"

#include "engine/CallFixedSize.h"
#include "engine/sparqlExpressions/SparqlExpressionValueGetters.h"
#include "util/Algorithm.h"

#ifdef __x86_64__
#include <cpuid.h>
#elif defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

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
// SIMDFilterEvaluator implementation (V2)
// Placeholder: delegates to scalar until P3E implements vectorization
// _____________________________________________________________________________

IdTable SIMDFilterEvaluator::evaluate(
    const IdTable& input,
    const sparqlExpression::SparqlExpressionPimpl& expression,
    sparqlExpression::EvaluationContext& context) const {
  // Fallback implementation: SIMD batch evaluation not yet available
  // See ROADMAP.md for EPIC 10 Phase 3E SIMD integration plan
  return scalarFallback_.evaluate(input, expression, context);
}

// _____________________________________________________________________________
// AdaptiveFilterEvaluator implementation (V3)
// Runtime CPU detection + selection at construction
// _____________________________________________________________________________

bool AdaptiveFilterEvaluator::detectSIMDSupport() {
#ifdef __x86_64__
  // x86_64: Check for AVX2 support via cpuid
  unsigned int eax, ebx, ecx, edx;
  if (__get_cpuid(7, &eax, &ebx, &ecx, &edx)) {
    return (ebx & (1 << 5)) != 0;  // AVX2 bit
  }
  return false;
#elif defined(__aarch64__)
  // ARM64: Check for ASIMD (NEON) support via getauxval
  unsigned long hwcaps = getauxval(AT_HWCAP);
  return (hwcaps & HWCAP_ASIMD) != 0;
#else
  // Unsupported architecture: no SIMD
  return false;
#endif
}

AdaptiveFilterEvaluator::AdaptiveFilterEvaluator() {
  // Selection at initialization (NOT hot path)
  bool simdAvailable = detectSIMDSupport();

  if (simdAvailable) {
    // SIMD supported: use vectorized evaluator
    impl_ = std::make_unique<SIMDFilterEvaluator>();
  } else {
    // SIMD not supported: fallback to scalar
    impl_ = std::make_unique<ScalarFilterEvaluator>();
  }

  // Note: This selection happens once at construction.
  // Hot path (evaluate()) uses virtual dispatch only, no branches.
}

// _____________________________________________________________________________
// FilterEvaluatorFactory implementation
// _____________________________________________________________________________

std::unique_ptr<FilterEvaluator> FilterEvaluatorFactory::create(
    EvaluatorType type) {
  switch (type) {
    case EvaluatorType::SCALAR:
      return std::make_unique<ScalarFilterEvaluator>();
    case EvaluatorType::SIMD:
      return std::make_unique<SIMDFilterEvaluator>();
    case EvaluatorType::ADAPTIVE:
    default:
      return std::make_unique<AdaptiveFilterEvaluator>();
  }
}
