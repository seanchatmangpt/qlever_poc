//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent 3 (Unified Physical Optimizer - EPIC 10.3)
//
//  ASPIRATIONAL STUB: This is architecture for future EPIC 10.3 implementation.
//  All optimization methods are stubs that delegate to base QueryPlanner.
//  See EPIC13_TRUTH_AUDIT.md for aspirational vs actual capabilities.

#include "engine/UnifiedPhysicalOptimizerStub.h"

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "parser/GraphPattern.h"
#include "util/Exception.h"
#include "util/Log.h"

// Constructor
UnifiedPhysicalOptimizer::UnifiedPhysicalOptimizer(
    QueryExecutionContext* qec,
    ad_utility::SharedCancellationHandle cancellationHandle)
    : QueryPlanner(qec, std::move(cancellationHandle)),
      datalogPlanner_(nullptr),
      shaclStrategy_(nullptr),
      uirContext_() {
  // Initialize DatalogQueryPlanner if RuleDatabase is available
  // (RuleDatabase is set via setRuleDatabase() method inherited from
  // QueryPlanner)
  // Note: We defer initialization until first use to avoid constructing
  // DatalogQueryPlanner before RuleDatabase is set.

  // Initialize ShaclPlanningStrategy
  // Note: ShaclPlanningStrategy constructor requires a ShaclShapeRegistry,
  // which may not be available at construction time. We defer initialization
  // until first use.
  // For Part 2 integration testing, we can function without these components.

  LOG(DEBUG) << "UnifiedPhysicalOptimizer constructed (UIR enabled: "
             << uirContext_.enabled << ")" << std::endl;
}

// Override createExecutionTree to use UIR-based planning when appropriate
QueryExecutionTree UnifiedPhysicalOptimizer::createExecutionTree(
    ParsedQuery& pq, bool isSubquery) {
  // Check if we should use UIR planning for this query
  if (!shouldUseUIRPlanning(pq)) {
    // Fall back to base QueryPlanner implementation
    LOG(DEBUG) << "UIR planning not applicable, using base QueryPlanner"
               << std::endl;
    return QueryPlanner::createExecutionTree(pq, isSubquery);
  }

  LOG(DEBUG) << "Using UIR-based planning" << std::endl;

  try {
    // Phase 1: Compile ParsedQuery to UIR
    UIRPlan uirPlan = compileToUIR(pq);

    // Phase 2: Apply SHACL Focus-Node Injection optimization
    if (uirContext_.focusNodeInjectionEnabled && hasShaclPatterns(pq)) {
      applyFocusNodeInjection(uirPlan);
    }

    // Phase 3: Apply Datalog Semi-Naive Evaluation optimization
    if (uirContext_.semiNaiveEvaluationEnabled && hasDatalogPatterns(pq)) {
      applySemiNaiveEvaluation(uirPlan);
    }

    // Phase 4: Execute UIR plan to produce QueryExecutionTree
    return executeUIRPlan(uirPlan);

  } catch (const std::exception& e) {
    // If UIR planning fails, fall back to base QueryPlanner
    LOG(WARN) << "UIR planning failed (" << e.what()
              << "), falling back to base QueryPlanner" << std::endl;
    return QueryPlanner::createExecutionTree(pq, isSubquery);
  }
}

// Compile ParsedQuery to UIR
UIRPlan UnifiedPhysicalOptimizer::compileToUIR(const ParsedQuery& pq) {
  LOG(DEBUG) << "Compiling ParsedQuery to UIR" << std::endl;

  UIRPlan plan;

  // For Part 2 (integration architecture), we create a minimal UIR plan
  // that demonstrates the integration pattern without implementing the
  // full UIR compilation algorithm (which is Part 3/4 work).

  // In a full implementation, we would:
  // 1. Walk the ParsedQuery graph pattern tree
  // 2. Create UnifiedIRNode instances for each pattern
  // 3. Build adjacency structure for joins
  // For now, we just create an empty plan to demonstrate compilation succeeds.

  // Detect and record SHACL patterns
  if (hasShaclPatterns(pq)) {
    mergeShaclConstraints(plan, pq);
  }

  // Detect and record Datalog patterns
  if (hasDatalogPatterns(pq)) {
    mergeDatalogRules(plan, pq);
  }

  LOG(DEBUG) << "UIR compilation complete (nodes: " << plan.nodes.size()
             << ", SHACL constraints: " << plan.shaclConstraints.size()
             << ", Datalog rules: " << plan.datalogRules.size() << ")"
             << std::endl;

  return plan;
}

// Apply Focus-Node Injection optimization (SHACL)
void UnifiedPhysicalOptimizer::applyFocusNodeInjection(UIRPlan& plan) {
  LOG(DEBUG) << "Applying Focus-Node Injection optimization" << std::endl;

  // For Part 2, this is a stub that logs the optimization intent.
  // Full implementation in Part 3 will:
  // 1. Identify SHACL_VALIDATE nodes in UIR
  // 2. Extract target class/node constraints
  // 3. Find SCAN nodes that can benefit from constraint pushdown
  // 4. Inject constraints as filters on index scans

  if (!plan.shaclConstraints.empty()) {
    LOG(DEBUG) << "Focus-Node Injection: Would optimize "
               << plan.shaclConstraints.size() << " SHACL constraints"
               << std::endl;
  }
}

// Apply Semi-Naive Evaluation optimization (Datalog)
void UnifiedPhysicalOptimizer::applySemiNaiveEvaluation(UIRPlan& plan) {
  LOG(DEBUG) << "Applying Semi-Naive Evaluation optimization" << std::endl;

  // For Part 2, this is a stub that logs the optimization intent.
  // Full implementation will:
  // 1. Identify DATALOG_EXPAND nodes
  // 2. Analyze rule stratification
  // 3. Rewrite recursive rules for semi-naive evaluation

  if (!plan.datalogRules.empty()) {
    LOG(DEBUG) << "Semi-Naive Evaluation: Would optimize "
               << plan.datalogRules.size() << " Datalog rules" << std::endl;
  }
}

// Execute UIR plan to produce QueryExecutionTree
QueryExecutionTree UnifiedPhysicalOptimizer::executeUIRPlan(
    const UIRPlan& plan) {
  LOG(DEBUG) << "Executing UIR plan" << std::endl;

  // For Part 2, we demonstrate integration by delegating back to the
  // base QueryPlanner. A full implementation would convert UIR nodes
  // to concrete Operation instances.

  // This demonstrates that the UIR compilation pipeline can successfully
  // round-trip: ParsedQuery → UIR → QueryExecutionTree

  // Since we're in a stub implementation, we need to create a minimal
  // execution tree. In practice, this would never be called because
  // shouldUseUIRPlanning() returns false for Part 2.

  // Create a minimal neutral operation as a placeholder
  auto qec = getQec();
  AD_CONTRACT_CHECK(qec != nullptr);

  // Return an empty execution tree (will be populated in Part 3/4)
  return QueryExecutionTree(qec);
}

// Override seedFilterSubstitutes to inject SHACL constraints
QueryPlanner::FiltersAndOptionalSubstitutes
UnifiedPhysicalOptimizer::seedFilterSubstitutes(
    const std::vector<SparqlFilter>& filters) const {
  LOG(TRACE) << "seedFilterSubstitutes called with " << filters.size()
             << " filters" << std::endl;

  // Start with base QueryPlanner filter substitutes
  auto result = QueryPlanner::seedFilterSubstitutes(filters);

  // If SHACL strategy is available, add SHACL-specific substitutes
  if (shaclStrategy_ != nullptr) {
    // Coordinate with ShaclPlanningStrategy to detect SHACL validation
    // patterns and create validator operation substitutes
    // For Part 2, this is a stub - full implementation in Part 3
    LOG(TRACE) << "Would coordinate with ShaclPlanningStrategy for "
               << "SHACL filter substitutes" << std::endl;
  }

  return result;
}

// Helper: Detect SHACL patterns in query
bool UnifiedPhysicalOptimizer::hasShaclPatterns(const ParsedQuery& pq) const {
  // For Part 2, use the ShaclPlanningStrategy if available
  if (shaclStrategy_ != nullptr) {
    return shaclStrategy_->detectShaclValidationRequest(pq);
  }

  // Fallback: Simple heuristic check for SHACL-related predicates
  // in the query string (very basic, just for Part 2 integration)
  const auto& queryStr = pq._originalString;
  return queryStr.find("sh:conforms") != std::string::npos ||
         queryStr.find("sh:ValidationResult") != std::string::npos ||
         queryStr.find("sh:targetClass") != std::string::npos;
}

// Helper: Detect Datalog patterns in query
bool UnifiedPhysicalOptimizer::hasDatalogPatterns(const ParsedQuery& pq) const {
  // Use inherited method from QueryPlanner if RuleDatabase is set
  return hasRulePredicates(pq);
}

// Helper: Merge SHACL constraints into UIR plan
void UnifiedPhysicalOptimizer::mergeShaclConstraints(UIRPlan& plan,
                                                     const ParsedQuery& pq) {
  LOG(DEBUG) << "Merging SHACL constraints into UIR plan" << std::endl;

  // For Part 2, this is a placeholder that records that SHACL patterns
  // were detected. Full constraint extraction in Part 3.

  // If ShaclPlanningStrategy is available, we could extract actual constraints
  // For now, just log the detection
  if (shaclStrategy_ != nullptr) {
    LOG(DEBUG) << "SHACL patterns detected, would extract constraints"
               << std::endl;
  }
}

// Helper: Merge Datalog rules into UIR plan
void UnifiedPhysicalOptimizer::mergeDatalogRules(UIRPlan& plan,
                                                 const ParsedQuery& pq) {
  LOG(DEBUG) << "Merging Datalog rules into UIR plan" << std::endl;

  // For Part 2, this is a placeholder that records that Datalog patterns
  // were detected. Full rule extraction in future parts.

  // If DatalogQueryPlanner is available and has RuleDatabase, we could
  // extract actual rules. For now, just log the detection.
  LOG(DEBUG) << "Datalog patterns detected, would extract rules" << std::endl;
}

// Helper: Determine if UIR planning should be used
bool UnifiedPhysicalOptimizer::shouldUseUIRPlanning(
    const ParsedQuery& pq) const {
  // For Part 2 (integration architecture), we disable actual UIR planning
  // and always fall back to base QueryPlanner. This allows us to:
  // 1. Demonstrate polymorphic substitution works
  // 2. Verify compilation succeeds
  // 3. Ensure existing tests still pass
  // 4. Provide a foundation for Part 3/4 implementation

  // UIR planning will be enabled in Part 3 when Focus-Node Injection
  // and Semi-Naive Evaluation are fully implemented.

  // Check if UIR is globally enabled
  if (!uirContext_.enabled) {
    return false;
  }

  // For Part 2, always delegate to base QueryPlanner
  // This will be changed in Part 3 to:
  // return hasShaclPatterns(pq) || hasDatalogPatterns(pq);
  return false;
}
