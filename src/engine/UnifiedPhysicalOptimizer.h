//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent 3 (Unified Physical Optimizer - EPIC 10.3)

#ifndef QLEVER_SRC_ENGINE_UNIFIEDPHYSICALOPTIMIZER_H
#define QLEVER_SRC_ENGINE_UNIFIEDPHYSICALOPTIMIZER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "engine/DatalogQueryPlanner.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "engine/UnifiedIRNode.h"
#include "engine/shacl/ShaclPlanningStrategy.h"
#include "engine/shacl/ShaclShape.h"
#include "parser/DatalogRule.h"
#include "parser/ParsedQuery.h"
#include "util/HashMap.h"
#include "util/HashSet.h"

// Forward declarations
class QueryExecutionContext;

// Unified Intermediate Representation (UIR) data structures
// Note: UnifiedIRNode is defined in UnifiedIRNode.h (Part 1)
// This file defines higher-level structures for UIR-based planning

/**
 * @brief UIR plan representing the complete query execution plan
 *
 * Contains a graph of UnifiedIRNode instances plus metadata about
 * SHACL constraints and Datalog rules that are part of the query.
 *
 * For Part 2 (integration architecture), this structure provides
 * a foundation for UIR-based optimization. The full graph structure
 * and traversal algorithms will be implemented in future parts.
 */
struct UIRPlan {
  // Root nodes of the UIR graph (may have multiple roots for disconnected
  // components)
  std::vector<qlever::unified::UnifiedIRNode> nodes;

  // All variables bound by this plan
  ad_utility::HashSet<Variable> boundVariables;

  // SHACL constraints detected in the query
  std::vector<shacl::ShaclConstraint> shaclConstraints;

  // Datalog rules detected in the query
  std::vector<DatalogRule> datalogRules;

  UIRPlan() = default;
};

/**
 * @brief UIR optimization context and configuration
 *
 * Controls which UIR optimizations are enabled and their parameters.
 */
struct UIRContext {
  // Enable Focus-Node Injection optimization (SHACL)
  bool focusNodeInjectionEnabled = true;

  // Enable Semi-Naive Evaluation optimization (Datalog)
  bool semiNaiveEvaluationEnabled = true;

  // Maximum recursion depth for Datalog rules
  size_t maxRecursionDepth = 100;

  // Enable UIR-based planning (can be disabled to fallback to base
  // QueryPlanner)
  bool enabled = true;
};

/**
 * @brief Unified Physical Optimizer (Agent 3 - EPIC 10.3)
 *
 * This class extends QueryPlanner to provide first-class support for SHACL
 * constraints and Datalog rules via a Unified Intermediate Representation
 * (UIR).
 *
 * Architecture:
 * - Inherits from QueryPlanner for polymorphic compatibility
 * - Internally coordinates DatalogQueryPlanner and ShaclPlanningStrategy
 * - Compiles ParsedQuery → UIR → QueryExecutionTree
 *
 * Key Optimizations:
 * - Focus-Node Injection: Pushes SHACL target constraints to index scans
 * - Semi-Naive Evaluation: Optimizes stratified Datalog recursion
 *
 * Integration:
 * - Drop-in replacement for QueryPlanner (polymorphic substitution)
 * - All existing QueryPlanner* usage continues to work unchanged
 *
 * Implementation Status:
 * - Part 2: Integration architecture (this file)
 * - Part 3: Focus-Node Injection algorithm (future)
 * - Part 4: Semantic equivalence validation (future)
 */
class UnifiedPhysicalOptimizer : public QueryPlanner {
 public:
  /**
   * @brief Construct UnifiedPhysicalOptimizer
   *
   * @param qec QueryExecutionContext (owned by caller, must outlive this
   * object)
   * @param cancellationHandle Cancellation token for query interruption
   */
  explicit UnifiedPhysicalOptimizer(
      QueryExecutionContext* qec,
      ad_utility::SharedCancellationHandle cancellationHandle);

  /**
   * @brief Destructor
   */
  ~UnifiedPhysicalOptimizer() override = default;

  /**
   * @brief Create execution tree using UIR-based planning
   *
   * Overrides QueryPlanner::createExecutionTree to:
   * 1. Check if UIR planning is enabled and query is suitable
   * 2. If yes: Compile ParsedQuery to UIR, optimize, and execute
   * 3. If no: Delegate to base QueryPlanner implementation
   *
   * This ensures backward compatibility while enabling UIR optimizations
   * when appropriate.
   *
   * @param pq Parsed query
   * @param isSubquery True if this is a subquery
   * @return Optimized execution tree
   */
  QueryExecutionTree createExecutionTree(ParsedQuery& pq,
                                         bool isSubquery = false) override;

  /**
   * @brief Compile ParsedQuery to Unified IR
   *
   * Converts SPARQL, SHACL, and Datalog patterns into a unified intermediate
   * representation that treats all three as first-class constructs.
   *
   * Algorithm (high-level):
   * 1. Walk ParsedQuery graph pattern tree
   * 2. Detect SHACL validation patterns (sh:conforms, sh:ValidationResult)
   * 3. Detect Datalog rule predicates (via RuleDatabase)
   * 4. Create UIRNode tree mirroring query structure
   * 5. Annotate with SHACL constraints and Datalog rules
   *
   * @param pq Parsed query
   * @return UIR plan ready for optimization
   */
  UIRPlan compileToUIR(const ParsedQuery& pq);

  /**
   * @brief Apply SHACL Focus-Node Injection optimization
   *
   * Pushes SHACL sh:targetClass and sh:targetNode constraints down to
   * index scan operations for early filtering.
   *
   * Algorithm (stub for Part 2, full implementation in Part 3):
   * 1. Identify SHACL_VALIDATE nodes in UIR
   * 2. Extract target class/node constraints
   * 3. Find SCAN nodes that can benefit from constraint pushdown
   * 4. Inject constraints as filters on index scans
   *
   * @param plan UIR plan to optimize (modified in-place)
   */
  void applyFocusNodeInjection(UIRPlan& plan);

  /**
   * @brief Apply Datalog Semi-Naive Evaluation optimization
   *
   * Optimizes stratified Datalog recursion using semi-naive evaluation:
   * - Identifies stratified rules (no mutual recursion)
   * - Computes fixed-point iteratively using differential updates
   * - Avoids redundant computation of unchanged tuples
   *
   * Algorithm (stub for Part 2, full implementation in future):
   * 1. Identify DATALOG_EXPAND nodes
   * 2. Analyze rule stratification
   * 3. Rewrite recursive rules for semi-naive evaluation
   *
   * @param plan UIR plan to optimize (modified in-place)
   */
  void applySemiNaiveEvaluation(UIRPlan& plan);

  /**
   * @brief Execute UIR plan and produce QueryExecutionTree
   *
   * Converts optimized UIR plan to concrete QueryExecutionTree by:
   * - Mapping UIR nodes to Operation instances (IndexScan, Join, etc.)
   * - Inserting ShaclValidator operations at optimal positions
   * - Inserting RuleExpansion operations for Datalog predicates
   *
   * @param plan Optimized UIR plan
   * @return Executable QueryExecutionTree
   */
  QueryExecutionTree executeUIRPlan(const UIRPlan& plan);

  /**
   * @brief Get UIR context (for testing/configuration)
   * @return Reference to UIR context
   */
  UIRContext& getUIRContext() { return uirContext_; }
  const UIRContext& getUIRContext() const { return uirContext_; }

 protected:
  /**
   * @brief Override filter substitutes to inject SHACL constraints
   *
   * Overrides QueryPlanner::seedFilterSubstitutes to:
   * - Detect SHACL validation patterns in filters
   * - Create ShaclValidator operation substitutes
   * - Coordinate with ShaclPlanningStrategy for cost estimation
   *
   * This allows SHACL constraints to be considered during the base
   * QueryPlanner's optimization process.
   *
   * @param filters SPARQL filters to analyze
   * @return Filters with SHACL substitutes added
   */
  FiltersAndOptionalSubstitutes seedFilterSubstitutes(
      const std::vector<SparqlFilter>& filters) const override;

 private:
  // Internal coordination: Datalog query planning
  std::unique_ptr<DatalogQueryPlanner> datalogPlanner_;

  // Internal coordination: SHACL planning strategy
  std::unique_ptr<shacl::ShaclPlanningStrategy> shaclStrategy_;

  // UIR-specific configuration
  UIRContext uirContext_;

  /**
   * @brief Detect if query contains SHACL patterns
   *
   * Checks for:
   * - Filters on sh:conforms
   * - Queries with sh:ValidationResult patterns
   * - Explicit SHACL validation functions
   *
   * @param pq Parsed query
   * @return True if SHACL patterns detected
   */
  bool hasShaclPatterns(const ParsedQuery& pq) const;

  /**
   * @brief Detect if query contains Datalog patterns
   *
   * Checks if any predicates in the query are defined by Datalog rules
   * in the RuleDatabase.
   *
   * @param pq Parsed query
   * @return True if Datalog patterns detected
   */
  bool hasDatalogPatterns(const ParsedQuery& pq) const;

  /**
   * @brief Merge SHACL constraints into UIR plan
   *
   * Analyzes ParsedQuery for SHACL validation requests and adds
   * SHACL_VALIDATE nodes to the UIR plan.
   *
   * @param plan UIR plan to modify
   * @param pq Parsed query
   */
  void mergeShaclConstraints(UIRPlan& plan, const ParsedQuery& pq);

  /**
   * @brief Merge Datalog rules into UIR plan
   *
   * Analyzes ParsedQuery for Datalog predicates and adds
   * DATALOG_EXPAND nodes to the UIR plan.
   *
   * @param plan UIR plan to modify
   * @param pq Parsed query
   */
  void mergeDatalogRules(UIRPlan& plan, const ParsedQuery& pq);

  /**
   * @brief Check if UIR planning should be used for this query
   *
   * Determines whether to use UIR-based planning or fall back to
   * base QueryPlanner. Factors:
   * - UIR enabled in context?
   * - Query contains SHACL or Datalog patterns?
   * - Query complexity suitable for UIR?
   *
   * @param pq Parsed query
   * @return True if UIR planning should be used
   */
  bool shouldUseUIRPlanning(const ParsedQuery& pq) const;
};

#endif  // QLEVER_SRC_ENGINE_UNIFIEDPHYSICALOPTIMIZER_H
