// Copyright 2026, QLever EPIC 10 Phase 3B
// Versioned filter evaluation interface for scalar/adaptive strategies
// EPIC 13: Removed fake SIMD implementation (was just delegating to scalar)

#ifndef QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H
#define QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H

#include <memory>

#include "engine/idTable/IdTable.h"
#include "engine/sparqlExpressions/SparqlExpression.h"

/**
 * @brief Versioned filter evaluation interface
 *
 * Provides abstraction for filter evaluation strategies.
 *
 * Versions:
 * - V1: ScalarEvaluator (existing Filter::computeFilterImpl logic)
 * - V2: AdaptiveEvaluator (currently uses ScalarEvaluator, reserved for future
 * optimizations)
 *
 * Invariants preserved:
 * - Immutability: Evaluators are stateless (pure evaluation)
 * - Determinism: Output is deterministic given identical inputs
 * - Atomicity: Evaluation completes atomically per row batch
 * - No External State: No side effects, context passed explicitly
 * - RAII: IdTable uses RAII allocators (unchanged)
 * - Backward Compat: Scalar path preserves existing behavior
 *
 * Selection at initialization (NOT hot path): Branchless property preserved
 */
class FilterEvaluator {
 public:
  virtual ~FilterEvaluator() = default;

  /**
   * @brief Evaluate filter expression on input table
   *
   * @param input Input IdTable (may be dynamic or static width)
   * @param expression SPARQL filter expression to evaluate
   * @param context Evaluation context (variable bindings, allocator, etc.)
   * @return Filtered IdTable (rows where expression evaluates to true)
   *
   * Invariant: Output is deterministic given identical inputs
   * Performance: Scalar O(n) where n = input.size()
   */
  virtual IdTable evaluate(
      const IdTable& input,
      const sparqlExpression::SparqlExpressionPimpl& expression,
      sparqlExpression::EvaluationContext& context) const = 0;

  /**
   * @brief Get evaluator version identifier
   * @return "SCALAR_V1" or "ADAPTIVE_V2"
   */
  virtual const char* version() const = 0;
};

/**
 * @brief V1: Scalar evaluator (existing logic from Filter::computeFilterImpl)
 *
 * Preserves 100% backward compatibility with existing Filter implementation.
 * Code directly extracted from Filter.cpp lines 131-227 (computeFilterImpl).
 */
class ScalarFilterEvaluator : public FilterEvaluator {
 public:
  IdTable evaluate(const IdTable& input,
                   const sparqlExpression::SparqlExpressionPimpl& expression,
                   sparqlExpression::EvaluationContext& context) const override;

  const char* version() const override { return "SCALAR_V1"; }
};

/**
 * @brief V2: Adaptive evaluator (reserved for future optimizations)
 *
 * Currently always uses ScalarFilterEvaluator.
 * Reserved for future SIMD or other optimizations when available.
 *
 * Future capabilities may include:
 * - CPU SIMD capability detection (AVX2, AVX512, ARM NEON)
 * - Filter expression complexity analysis
 * - Automatic selection based on table size
 *
 * Selection is immutable after construction (preserves branchless property).
 * Hot path uses virtual dispatch only (no conditional branches).
 */
class AdaptiveFilterEvaluator : public FilterEvaluator {
 public:
  /**
   * @brief Construct adaptive evaluator
   *
   * Currently always selects ScalarFilterEvaluator.
   * Future versions may detect CPU capabilities and select optimized
   * implementations when available.
   */
  AdaptiveFilterEvaluator();

  IdTable evaluate(
      const IdTable& input,
      const sparqlExpression::SparqlExpressionPimpl& expression,
      sparqlExpression::EvaluationContext& context) const override {
    // Virtual dispatch to selected implementation (no branches)
    return impl_->evaluate(input, expression, context);
  }

  const char* version() const override { return "ADAPTIVE_V2"; }

  // For testing: query which implementation was selected
  const FilterEvaluator* getImplementation() const { return impl_.get(); }

 private:
  std::unique_ptr<FilterEvaluator> impl_;
};

/**
 * @brief Factory for creating filter evaluators based on runtime config
 *
 * Centralizes evaluator selection logic for easy testing and configuration.
 */
class FilterEvaluatorFactory {
 public:
  enum class EvaluatorType {
    SCALAR,   // Force scalar (for testing, legacy systems)
    ADAPTIVE  // Auto-detect (default, production use - currently same as
              // SCALAR)
  };

  static std::unique_ptr<FilterEvaluator> create(
      EvaluatorType type = EvaluatorType::ADAPTIVE);
};

#endif  // QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H
