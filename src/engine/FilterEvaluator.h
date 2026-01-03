// Copyright 2026, QLever EPIC 10 Phase 3B
// Versioned filter evaluation interface for scalar/SIMD/adaptive strategies
// BB80/20: Interface abstraction for P3E SIMD handoff (Week 7)

#ifndef QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H
#define QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H

#include <memory>

#include "engine/idTable/IdTable.h"
#include "engine/sparqlExpressions/SparqlExpression.h"

/**
 * @brief Versioned filter evaluation interface (ZONE 3 Strategy)
 *
 * EPIC 10 P3B deliverable: Versioned evaluation with fallback
 * Enables P3E (SIMD Integration) to add vectorized evaluator without
 * modifying existing scalar code.
 *
 * Versions:
 * - V1: ScalarEvaluator (existing Filter::computeFilterImpl logic)
 * - V2: SIMDEvaluator (batch vectorized, P3E Week 7 handoff target)
 * - V3: AdaptiveFallback (runtime CPU detection, selects V1 or V2)
 *
 * Invariants (AX-1 to AX-6 preserved):
 * - AX-1 (Immutability): Evaluators are stateless (pure evaluation)
 * - AX-2 (Determinism): Scalar == SIMD results (bit-identical validation)
 * - AX-3 (Atomicity): Evaluation completes atomically per row batch
 * - AX-4 (No External State): No side effects, context passed explicitly
 * - AX-5 (RAII): IdTable uses RAII allocators (unchanged)
 * - AX-6 (Backward Compat): Scalar path preserves existing behavior
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
   * Performance: Scalar O(n), SIMD O(n/vectorWidth) where n = input.size()
   */
  virtual IdTable evaluate(
      const IdTable& input,
      const sparqlExpression::SparqlExpressionPimpl& expression,
      sparqlExpression::EvaluationContext& context) const = 0;

  /**
   * @brief Get evaluator version identifier
   * @return "SCALAR_V1", "SIMD_V2", or "ADAPTIVE_V3"
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
 * @brief V2: SIMD batch evaluator (P3E handoff target, Week 7)
 *
 * ASPIRATIONAL STUB: SIMD vectorization not yet implemented.
 * This class currently delegates all work to ScalarFilterEvaluator.
 *
 * P3E Requirements (from COLLISION_ZONE_RESOLUTIONS.md):
 * - Consume IdTableAOS interface (ZONE 1 dependency)
 * - Implement batch evaluation (4-8 rows per SIMD register)
 * - Guarantee bit-identical results vs. ScalarFilterEvaluator
 * - Performance target: 2-4x faster than scalar on supported CPUs
 *
 * **WARNING**: Despite the name, this evaluator provides NO SIMD acceleration.
 * It returns scalar results and has the same performance as
 * ScalarFilterEvaluator. Do not use this class if you expect SIMD performance
 * gains.
 *
 * Current status: STUB - delegates to scalar fallback (see FilterEvaluator.cpp
 * line 101)
 */
class SIMDFilterEvaluator : public FilterEvaluator {
 public:
  IdTable evaluate(const IdTable& input,
                   const sparqlExpression::SparqlExpressionPimpl& expression,
                   sparqlExpression::EvaluationContext& context) const override;

  const char* version() const override { return "SIMD_V2"; }

 private:
  // Fallback implementation: SIMD vectorization not yet available
  // See ROADMAP.md for EPIC 10 Phase 3E SIMD integration plan
  ScalarFilterEvaluator scalarFallback_;
};

/**
 * @brief V3: Adaptive fallback (runtime CPU detection)
 *
 * Selects SIMD or Scalar at construction time based on:
 * - CPU SIMD capability detection (AVX2, AVX512, ARM NEON)
 * - Filter expression complexity (some patterns don't vectorize well)
 * - Table size (SIMD overhead not worth it for small tables < 1K rows)
 *
 * Selection is immutable after construction (preserves branchless property).
 * Hot path uses virtual dispatch only (no conditional branches).
 */
class AdaptiveFilterEvaluator : public FilterEvaluator {
 public:
  /**
   * @brief Construct adaptive evaluator (selection at initialization)
   *
   * Decision logic (executed once, not in hot path):
   * 1. Detect CPU SIMD support (cpuid, getauxval, etc.)
   * 2. If SIMD available && expression is vectorizable: use SIMDFilterEvaluator
   * 3. Otherwise: use ScalarFilterEvaluator
   *
   * This ensures hot path has zero conditional branches (just virtual
   * dispatch).
   */
  AdaptiveFilterEvaluator();

  IdTable evaluate(
      const IdTable& input,
      const sparqlExpression::SparqlExpressionPimpl& expression,
      sparqlExpression::EvaluationContext& context) const override {
    // Virtual dispatch to selected implementation (no branches)
    return impl_->evaluate(input, expression, context);
  }

  const char* version() const override { return "ADAPTIVE_V3"; }

  // For testing: query which implementation was selected
  const FilterEvaluator* getImplementation() const { return impl_.get(); }

 private:
  std::unique_ptr<FilterEvaluator> impl_;

  // SIMD capability detection (compile-time + runtime)
  static bool detectSIMDSupport();
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
    SIMD,     // Force SIMD (for testing, assumes CPU support)
    ADAPTIVE  // Auto-detect (default, production use)
  };

  static std::unique_ptr<FilterEvaluator> create(
      EvaluatorType type = EvaluatorType::ADAPTIVE);
};

#endif  // QLEVER_SRC_ENGINE_FILTER_EVALUATOR_H
