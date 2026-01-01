#ifndef QLEVER_ENGINE_SHACL_SHACLPLANNINGSTRATEGY_H
#define QLEVER_ENGINE_SHACL_SHACLPLANNINGSTRATEGY_H

#include <memory>
#include <optional>
#include <vector>

#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "engine/shacl/ShaclValidator.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlFilter.h"

namespace shacl {

// Forward declarations
class ShaclValidator;
class ShaclShapeRegistry;

/**
 * @brief SHACL-aware query planning strategy adapter
 *
 * This class provides an integration layer between QueryPlanner and SHACL
 * validation. It analyzes queries to detect SHACL validation requests,
 * creates ShaclValidator operations at optimal positions, and implements
 * constraint pushdown optimizations.
 *
 * IMPORTANT: This is an adapter layer that does NOT modify QueryPlanner
 * directly. Instead, it provides utility functions that can be called
 * to enhance plans with SHACL validation.
 *
 * Usage:
 *   ShaclPlanningStrategy strategy(shapeRegistry, qec);
 *   auto plans = queryPlanner.createExecutionTrees(parsedQuery);
 *   auto enhancedPlans = strategy.addShaclValidationIfNeeded(plans,
 * parsedQuery);
 */
class ShaclPlanningStrategy {
 public:
  /**
   * @brief Construct a SHACL planning strategy
   * @param shapeRegistry The SHACL shape registry to use for validation
   * @param qec The query execution context
   */
  explicit ShaclPlanningStrategy(const ShaclShapeRegistry* shapeRegistry,
                                 QueryExecutionContext* qec);

  /**
   * @brief Detect if a query contains SHACL validation patterns
   *
   * Checks for special predicates or patterns that indicate SHACL validation:
   * - Filters on sh:conforms
   * - Queries with sh:ValidationResult patterns
   * - Explicit SHACL validation functions
   *
   * @param pq The parsed query to analyze
   * @return True if SHACL validation is detected
   */
  bool detectShaclValidationRequest(const parsedQuery::ParsedQuery& pq) const;

  /**
   * @brief Add SHACL validation operations to plans if needed
   *
   * Analyzes the parsed query and existing plans, then inserts ShaclValidator
   * operations at optimal positions if SHACL validation is required.
   *
   * @param plans The existing execution plans from QueryPlanner
   * @param pq The parsed query being planned
   * @return Enhanced plans with SHACL validation operations inserted
   */
  std::vector<QueryPlanner::SubtreePlan> addShaclValidationIfNeeded(
      std::vector<QueryPlanner::SubtreePlan> plans,
      const parsedQuery::ParsedQuery& pq) const;

  /**
   * @brief Create a SubtreePlan with a ShaclValidator operation
   *
   * @param subtree The input subtree producing resources to validate
   * @param shapeId Optional specific shape ID to validate against
   * @param resourceColumnIndex The column containing resources to validate
   * @return SubtreePlan containing a ShaclValidator operation
   */
  QueryPlanner::SubtreePlan createValidationPlan(
      std::shared_ptr<QueryExecutionTree> subtree,
      std::optional<std::string> shapeId = std::nullopt,
      ColumnIndex resourceColumnIndex = 0) const;

  /**
   * @brief Estimate the cost of SHACL validation for a plan
   *
   * Provides shape-aware cost estimation that considers:
   * - Number of property shapes to validate
   * - Complexity of constraints
   * - Input result size
   *
   * @param plan The plan to estimate validation cost for
   * @param shapeId Optional specific shape (if known)
   * @return Estimated cost of adding SHACL validation
   */
  size_t estimateValidationCost(
      const QueryPlanner::SubtreePlan& plan,
      std::optional<std::string> shapeId = std::nullopt) const;

  /**
   * @brief Try to push shape constraints into index scans
   *
   * Analyzes SHACL shape constraints to see if they can be pushed down
   * into index scan operations for early filtering. This is an important
   * optimization to reduce intermediate result sizes.
   *
   * For example, if a shape requires sh:targetClass :Person, this can
   * be pushed into the index scan as a type filter.
   *
   * @param plan The plan to optimize
   * @param shapeId The shape whose constraints to push down
   * @return Optimized plan with constraints pushed down (or original if not
   * possible)
   */
  QueryPlanner::SubtreePlan tryPushdownConstraints(
      QueryPlanner::SubtreePlan plan, const std::string& shapeId) const;

  /**
   * @brief Find the optimal insertion point for SHACL validation
   *
   * Analyzes the execution plan to determine where ShaclValidator should
   * be inserted for maximum efficiency. Considers:
   * - Which columns contain resources to validate
   * - Where property data is available
   * - Cost of validation at different positions
   *
   * @param plan The execution plan to analyze
   * @param boundVariables Variables bound by the plan
   * @return Index in the plan tree where validation should be inserted
   */
  size_t findOptimalValidationPosition(
      const QueryPlanner::SubtreePlan& plan,
      const ad_utility::HashSet<Variable>& boundVariables) const;

  /**
   * @brief Extract shape constraints that can be used for index filtering
   *
   * Analyzes a SHACL shape to extract constraints that can be translated
   * into index scan filters:
   * - sh:targetClass -> type filters
   * - sh:nodeKind IRI -> filter for IRIs only
   * - sh:pattern -> can sometimes be pushed to index
   *
   * @param shapeId The shape to analyze
   * @return List of filter expressions that can be pushed to index scans
   */
  std::vector<SparqlFilter> extractPushableConstraints(
      const std::string& shapeId) const;

  /**
   * @brief Check if a plan contains variables needed for shape validation
   *
   * @param plan The plan to check
   * @param shapeId The shape requiring validation
   * @return True if the plan binds all variables needed for validation
   */
  bool planSupportsShapeValidation(const QueryPlanner::SubtreePlan& plan,
                                   const std::string& shapeId) const;

  /**
   * @brief Reorder validation operations for optimal efficiency
   *
   * When multiple shapes need validation, determines the optimal order
   * based on:
   * - Constraint complexity (simple first)
   * - Expected selectivity (selective first)
   * - Data availability
   *
   * @param validationPlans Multiple validation plans to reorder
   * @return Reordered plans for optimal efficiency
   */
  std::vector<QueryPlanner::SubtreePlan> reorderValidationOperations(
      std::vector<QueryPlanner::SubtreePlan> validationPlans) const;

  /**
   * @brief Create property column mappings for efficient validation
   *
   * Analyzes the execution plan to identify which columns contain which
   * properties, enabling efficient property shape validation.
   *
   * @param plan The execution plan
   * @param shapeId The shape requiring property mappings
   * @return Map from property IRI to column index
   */
  std::unordered_map<std::string, ColumnIndex> createPropertyColumnMapping(
      const QueryPlanner::SubtreePlan& plan, const std::string& shapeId) const;

  /**
   * @brief Enable/disable SHACL planning optimizations
   * @param enabled Whether to enable SHACL planning
   */
  void setEnabled(bool enabled) { enabled_ = enabled; }

  /**
   * @brief Check if SHACL planning is enabled
   * @return True if enabled
   */
  bool isEnabled() const { return enabled_; }

 private:
  // The SHACL shape registry (non-owning)
  const ShaclShapeRegistry* shapeRegistry_;

  // Query execution context
  QueryExecutionContext* qec_;

  // Whether SHACL planning is enabled
  bool enabled_ = true;

  /**
   * @brief Estimate constraint complexity for cost calculation
   * @param constraint The constraint to analyze
   * @return Complexity score (higher = more expensive)
   */
  size_t estimateConstraintComplexity(const ShaclConstraint& constraint) const;

  /**
   * @brief Check if a constraint can be pushed to an index scan
   * @param constraint The constraint to check
   * @return True if pushable to index
   */
  bool isConstraintPushable(const ShaclConstraint& constraint) const;

  /**
   * @brief Detect SHACL-related filters in the query
   * @param filters The filters to analyze
   * @return True if SHACL validation filters detected
   */
  bool hasShaclFilters(const std::vector<SparqlFilter>& filters) const;

  /**
   * @brief Extract target shape ID from query patterns
   * @param pq The parsed query
   * @return Optional shape ID if explicitly specified
   */
  std::optional<std::string> extractTargetShapeFromQuery(
      const parsedQuery::ParsedQuery& pq) const;
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLPLANNINGSTRATEGY_H
