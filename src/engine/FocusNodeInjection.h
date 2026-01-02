// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Agent 3 (EPIC 10.3 - Focus-Node Injection)

#ifndef QLEVER_SRC_ENGINE_FOCUSNODEINJECTION_H
#define QLEVER_SRC_ENGINE_FOCUSNODEINJECTION_H

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "engine/QueryPlanner.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "parser/SparqlFilter.h"

namespace qlever {

/**
 * @brief Focus-Node Injection: SHACL constraint push-down optimization
 *
 * This class implements the Focus-Node Injection algorithm for optimizing
 * SPARQL queries with SHACL validation. The algorithm pushes SHACL shape
 * constraints into index scans and joins to reduce intermediate result
 * cardinality.
 *
 * Key optimization: Transform post-hoc validation into early filtering by
 * injecting constraints from sh:targetClass, sh:targetNode, and property
 * constraints directly into index scan operations.
 *
 * EPIC 10.3 - Agent 3 Part 3
 * Specification: docs/epic-10-3/PATCH_2_FOCUS_NODE_INJECTION.md
 */
class FocusNodeInjection {
 public:
  /**
   * @brief Apply Focus-Node Injection optimization to a query plan
   *
   * Pushes SHACL shape constraints into index scans and joins for early
   * filtering. Reduces intermediate result cardinality by applying type
   * filters, datatype constraints, and range constraints at scan time.
   *
   * Monoidal composition: Idempotent and commutative with other optimizations.
   *
   * @param plan Initial execution plan from QueryPlanner
   * @param shapes SHACL shape registry containing constraint definitions
   * @return Optimized plan with injected constraints
   */
  static QueryPlanner::SubtreePlan optimizeWithShaclConstraints(
      QueryPlanner::SubtreePlan plan, const shacl::ShaclShapeRegistry* shapes);

  /**
   * @brief Extract filters from SHACL target definitions
   *
   * Converts sh:targetClass and sh:targetNode into SPARQL filter expressions
   * that can be pushed into index scans.
   *
   * @param shape The SHACL shape to extract targets from
   * @return Vector of filter expressions for target constraints
   */
  static std::vector<SparqlFilter> extractTargetFilters(
      const shacl::NodeShape& shape);

  /**
   * @brief Extract property constraints that can be pushed to index scans
   *
   * Analyzes property shapes to identify constraints that are "pushable"
   * (can be evaluated during index scan without aggregation or joins).
   *
   * Pushable constraints:
   * - sh:datatype (value type filters)
   * - sh:nodeKind (IRI/Literal/BlankNode filters)
   * - sh:minInclusive / sh:maxInclusive (range filters)
   * - sh:in (set membership)
   *
   * Non-pushable constraints (require post-scan evaluation):
   * - sh:minCount / sh:maxCount (aggregation)
   * - sh:pattern (regex, expensive)
   * - sh:unique (global uniqueness check)
   *
   * @param shape The SHACL shape to analyze
   * @return Vector of pushable filter expressions
   */
  static std::vector<SparqlFilter> extractPushablePropertyConstraints(
      const shacl::NodeShape& shape);

  /**
   * @brief Check if an index scan matches a SHACL target class
   *
   * Determines whether an index scan operation can benefit from target
   * class constraint injection.
   *
   * @param scan The index scan operation to check
   * @param targetClasses SHACL target classes from shape
   * @return True if scan matches and can be optimized
   */
  static bool scanMatchesTarget(const Operation* scan,
                                const std::vector<std::string>& targetClasses);

  /**
   * @brief Estimate selectivity of a SHACL constraint
   *
   * Estimates the fraction of rows that will pass a given constraint.
   * Used for cost-based optimization and scan reordering.
   *
   * Selectivity table (from specification):
   * - sh:datatype: 0.95 (5% rejected)
   * - sh:nodeKind IRI: 0.90 (10% rejected)
   * - sh:minInclusive/maxInclusive: 0.80 (20% rejected)
   * - sh:in: |allowed_set| / |universe|
   * - sh:pattern: 0.50 (highly variable)
   *
   * @param constraint The SHACL constraint to estimate
   * @return Selectivity estimate (0.0 to 1.0)
   */
  static double estimateConstraintSelectivity(
      const shacl::ShaclConstraint& constraint);

  /**
   * @brief Compute effectiveness metric for constraint injection
   *
   * Effectiveness = (RowsEliminated × AvgDownstreamCost) / InjectionCost
   *
   * Interpretation:
   * - > 10: High value (inject immediately)
   * - 2-10: Moderate value (inject if cardinality large)
   * - < 2: Low value (skip injection)
   *
   * @param inputCardinality Number of input rows
   * @param selectivity Constraint selectivity (fraction passing)
   * @param downstreamCost Average cost per row in downstream ops
   * @param injectionCost Cost per row to evaluate constraint
   * @return Effectiveness ratio (benefit/cost)
   */
  static double computeEffectiveness(size_t inputCardinality,
                                     double selectivity, size_t downstreamCost,
                                     size_t injectionCost);

 private:
  /**
   * @brief Check if a constraint type is pushable to index scans
   *
   * @param type The constraint type to check
   * @return True if constraint can be pushed to index scan
   */
  static bool isConstraintPushable(shacl::ConstraintType type);

  /**
   * @brief Translate SHACL constraint to SPARQL filter expression
   *
   * Converts SHACL constraint semantics into equivalent SPARQL filter.
   *
   * @param path Property path (IRI)
   * @param constraint The SHACL constraint
   * @return SPARQL filter expression
   */
  static SparqlFilter translateConstraintToFilter(
      const std::string& path, const shacl::ShaclConstraint& constraint);

  /**
   * @brief Reorder index scans by estimated selectivity
   *
   * Sorts scans so most selective (lowest selectivity) execute first,
   * minimizing intermediate result sizes.
   *
   * @param plan The query plan to reorder
   * @return Reordered plan
   */
  static QueryPlanner::SubtreePlan reorderBySelectivity(
      QueryPlanner::SubtreePlan plan);
};

}  // namespace qlever

#endif  // QLEVER_SRC_ENGINE_FOCUSNODEINJECTION_H
