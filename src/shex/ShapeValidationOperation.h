// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: AI Assistant (Claude Code)

#ifndef QLEVER_SRC_SHEX_SHAPEVALIDATIONOPERATION_H
#define QLEVER_SRC_SHEX_SHAPEVALIDATIONOPERATION_H

#include <memory>
#include <vector>

#include "engine/Operation.h"
#include "engine/QueryExecutionTree.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Variable.h"
#include "shex/ShapeSchemaManager.h"

namespace shex {

/**
 * @brief Operation that wraps another operation with shape validation
 *
 * This operation validates query results against ShEx shape schemas.
 * It supports three validation modes:
 * - STRICT: Throw exception on validation failure
 * - LAX: Filter out invalid bindings from results
 * - REPORT: Add validation metadata column to results
 *
 * Integration with query engine:
 * - Hooks into Operation lifecycle for pre/post execution validation
 * - Provides optimization hints to QueryPlanner
 * - Seamlessly integrates with existing caching and runtime tracking
 */
class ShapeValidationOperation : public Operation {
 public:
  /**
   * @brief Construct a shape validation operation
   *
   * @param qec Query execution context
   * @param subtree The operation to validate
   * @param targetVariable The variable to validate
   * @param shapeId The shape to validate against
   * @param config Validation configuration
   */
  ShapeValidationOperation(QueryExecutionContext* qec,
                           std::shared_ptr<QueryExecutionTree> subtree,
                           Variable targetVariable, Iri shapeId,
                           ValidationConfig config = ValidationConfig{});

  // Get all child execution trees
  std::vector<QueryExecutionTree*> getChildren() override {
    return {subtree_.get()};
  }

  // Get a unique cache key for this operation
  std::string getCacheKeyImpl() const override;

  // Get a human-readable descriptor
  std::string getDescriptor() const override;

  // Get result width (may add validation report column)
  size_t getResultWidth() const override;

  // Get cost estimate (includes validation overhead)
  size_t getCostEstimate() override;

  // Get size estimate before limit
  uint64_t getSizeEstimateBeforeLimit() override;

  // Get column multiplicity
  float getMultiplicity(size_t col) override;

  // Check if result is known to be empty
  bool knownEmptyResult() override;

  // Get sorted columns
  std::vector<ColumnIndex> resultSortedOn() const override;

  // Get optimization hints for query planner
  const ShapeOptimizationHints& getOptimizationHints() const {
    return hints_;
  }

  // Check if this operation can benefit from limit push-down
  bool supportsLimitOffset() const override { return config_.mode == ValidationConfig::Mode::LAX; }

 private:
  // Compute the actual validation result
  Result computeResult(bool requestLaziness) override;

  // Compute variable to column mapping
  VariableToColumnMap computeVariableToColumnMap() const override;

  // Clone this operation
  std::unique_ptr<Operation> cloneImpl() const override;

  // Validate a single row according to shape constraints
  bool validateRow(const IdTable& table, size_t rowIdx) const;

  // Apply STRICT mode validation (throws on failure)
  Result applyStrictValidation(Result inputResult);

  // Apply LAX mode validation (filters invalid rows)
  Result applyLaxValidation(Result inputResult);

  // Apply REPORT mode validation (adds validation column)
  Result applyReportValidation(Result inputResult);

  // Helper: Get shape schema manager from context
  ShapeSchemaManager* getShapeSchemaManager() const;

  // Helper: Validate all triple constraints for a node
  bool validateTripleConstraints(
      const std::vector<TripleConstraint>& constraints, Id nodeId) const;

  // Helper: Validate node constraint
  bool validateNodeConstraint(const NodeConstraint& constraint,
                              Id nodeId) const;

  // Helper: Count occurrences of a predicate for a node
  size_t countPredicateOccurrences(Id nodeId, const Iri& predicate) const;

  // The subtree to validate
  std::shared_ptr<QueryExecutionTree> subtree_;

  // Variable to validate
  Variable targetVariable_;

  // Shape to validate against
  Iri shapeId_;

  // Validation configuration
  ValidationConfig config_;

  // Cached optimization hints
  ShapeOptimizationHints hints_;

  // Cached shape expression
  const ShapeExpression* cachedShape_ = nullptr;
};

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_SHAPEVALIDATIONOPERATION_H
