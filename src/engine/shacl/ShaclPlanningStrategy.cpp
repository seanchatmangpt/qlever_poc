#include "ShaclPlanningStrategy.h"

#include <algorithm>
#include <limits>

#include "engine/Filter.h"
#include "engine/IndexScan.h"
#include "parser/GraphPatternOperation.h"
#include "parser/TripleComponent.h"
#include "rdfTypes/Variable.h"

namespace shacl {

namespace {

// SHACL namespace constants
constexpr const char* SHACL_NS = "http://www.w3.org/ns/shacl#";
constexpr const char* SHACL_CONFORMS = "http://www.w3.org/ns/shacl#conforms";
constexpr const char* SHACL_VALIDATION_RESULT =
    "http://www.w3.org/ns/shacl#ValidationResult";

// Helper to create a SubtreePlan from an operation
template <typename Op>
QueryPlanner::SubtreePlan makeSubtreePlan(std::shared_ptr<Op> operation) {
  auto* qec = operation->getExecutionContext();
  return QueryPlanner::SubtreePlan(qec, std::move(operation));
}

}  // namespace

// _____________________________________________________________________________
ShaclPlanningStrategy::ShaclPlanningStrategy(
    const ShaclShapeRegistry* shapeRegistry, QueryExecutionContext* qec)
    : shapeRegistry_(shapeRegistry), qec_(qec) {
  AD_CONTRACT_CHECK(shapeRegistry != nullptr);
  AD_CONTRACT_CHECK(qec != nullptr);
}

// _____________________________________________________________________________
bool ShaclPlanningStrategy::detectShaclValidationRequest(
    const parsedQuery::ParsedQuery& pq) const {
  if (!enabled_ || !shapeRegistry_->isEnabled()) {
    return false;
  }

  // Check for SHACL-related filters
  if (hasShaclFilters(pq._rootGraphPattern._filters)) {
    return true;
  }

  // Check for SHACL validation patterns in the graph pattern
  // Look for patterns like: ?x sh:conforms ?result
  for (const auto& child : pq._rootGraphPattern._graphPatterns) {
    if (std::holds_alternative<parsedQuery::BasicGraphPattern>(child)) {
      const auto& bgp = std::get<parsedQuery::BasicGraphPattern>(child);
      for (const auto& triple : bgp._triples) {
        // Check if predicate is a SHACL validation predicate
        if (triple.p_.isIri()) {
          const auto& predicateIri =
              triple.p_.getIri().toStringRepresentation();
          if (predicateIri.find(SHACL_NS) != std::string::npos) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

// _____________________________________________________________________________
std::vector<QueryPlanner::SubtreePlan>
ShaclPlanningStrategy::addShaclValidationIfNeeded(
    std::vector<QueryPlanner::SubtreePlan> plans,
    const parsedQuery::ParsedQuery& pq) const {
  if (!detectShaclValidationRequest(pq)) {
    return plans;  // No SHACL validation needed
  }

  // Extract target shape if specified in query
  auto targetShape = extractTargetShapeFromQuery(pq);

  // Create enhanced plans with SHACL validation
  std::vector<QueryPlanner::SubtreePlan> enhancedPlans;
  enhancedPlans.reserve(plans.size());

  for (auto& plan : plans) {
    // Try constraint pushdown first
    QueryPlanner::SubtreePlan optimizedPlan = plan;
    if (targetShape.has_value()) {
      optimizedPlan =
          tryPushdownConstraints(std::move(optimizedPlan), targetShape.value());
    }

    // Create validation plan on top of optimized plan
    auto validationPlan =
        createValidationPlan(optimizedPlan._qet, targetShape, 0);

    enhancedPlans.push_back(std::move(validationPlan));
  }

  // Reorder validation operations for efficiency
  return reorderValidationOperations(std::move(enhancedPlans));
}

// _____________________________________________________________________________
QueryPlanner::SubtreePlan ShaclPlanningStrategy::createValidationPlan(
    std::shared_ptr<QueryExecutionTree> subtree,
    std::optional<std::string> shapeId, ColumnIndex resourceColumnIndex) const {
  // Create property column mapping if we have a specific shape
  std::unordered_map<std::string, ColumnIndex> propertyColumns;
  if (shapeId.has_value()) {
    propertyColumns = createPropertyColumnMapping(
        QueryPlanner::SubtreePlan(qec_, subtree), shapeId.value());
  }

  // Create the ShaclValidator operation
  auto validator = std::make_shared<ShaclValidator>(
      qec_, subtree, shapeRegistry_, resourceColumnIndex, shapeId);

  // Wrap in a SubtreePlan
  QueryPlanner::SubtreePlan plan(qec_, validator);

  return plan;
}

// _____________________________________________________________________________
size_t ShaclPlanningStrategy::estimateValidationCost(
    const QueryPlanner::SubtreePlan& plan,
    std::optional<std::string> shapeId) const {
  // Base cost is the subtree cost
  size_t baseCost = plan.getCostEstimate();
  size_t resultSize = plan.getSizeEstimate();

  // Add validation overhead
  size_t validationCost = 0;

  if (shapeId.has_value()) {
    // Cost for specific shape
    const auto* shape = shapeRegistry_->getShape(shapeId.value());
    if (shape != nullptr) {
      // Cost per property shape
      for (const auto& propShape : shape->propertyShapes) {
        size_t propCost = 0;
        for (const auto& constraint : propShape.constraints) {
          propCost += estimateConstraintComplexity(constraint);
        }
        validationCost += propCost;
      }

      // Cost per node constraint
      for (const auto& constraint : shape->nodeConstraints) {
        validationCost += estimateConstraintComplexity(constraint);
      }

      // Multiply by result size (validate each row)
      validationCost *= resultSize;
    } else {
      // Shape not found, use default cost estimate
      validationCost = resultSize;
    }
  } else {
    // Cost for all shapes (more expensive)
    auto allShapes = shapeRegistry_->getAllShapes();
    validationCost = resultSize * allShapes.size() * 10;  // Rough estimate
  }

  return baseCost + validationCost;
}

// _____________________________________________________________________________
QueryPlanner::SubtreePlan ShaclPlanningStrategy::tryPushdownConstraints(
    QueryPlanner::SubtreePlan plan, const std::string& shapeId) const {
  const auto* shape = shapeRegistry_->getShape(shapeId);
  if (shape == nullptr) {
    return plan;  // Can't optimize without shape info
  }

  // Extract pushable constraints from the shape
  auto pushableFilters = extractPushableConstraints(shapeId);

  if (pushableFilters.empty()) {
    return plan;  // Nothing to push down
  }

  // Future enhancement: In a full implementation, we would modify the plan's
  // index scans to include these filters. For now, we return the original plan.
  // This would require deeper integration with QueryPlanner's internal
  // structures, which we're avoiding per the requirements.

  return plan;
}

// _____________________________________________________________________________
size_t ShaclPlanningStrategy::findOptimalValidationPosition(
    const QueryPlanner::SubtreePlan& plan,
    const ad_utility::HashSet<Variable>& boundVariables) const {
  // In a simple strategy, validate as late as possible (after all data
  // is available). In practice, we'd analyze the plan structure to find
  // the best position.

  // For now, validate at the top of the plan
  return 0;
}

// _____________________________________________________________________________
std::vector<SparqlFilter> ShaclPlanningStrategy::extractPushableConstraints(
    const std::string& shapeId) const {
  std::vector<SparqlFilter> filters;

  const auto* shape = shapeRegistry_->getShape(shapeId);
  if (shape == nullptr) {
    return filters;
  }

  // Extract sh:targetClass constraints
  // These can be pushed down as type filters
  for (const auto& targetClass : shape->targetClasses) {
    // In a full implementation, we would create a SPARQL filter
    // equivalent to: FILTER(?x a <targetClass>)
    // For now, we just collect the constraint information
  }

  // Extract sh:nodeKind IRI constraint
  // This can filter out non-IRI nodes early
  for (const auto& constraint : shape->nodeConstraints) {
    if (isConstraintPushable(constraint)) {
      // Create appropriate filter expression
      // (implementation would depend on constraint type)
    }
  }

  return filters;
}

// _____________________________________________________________________________
bool ShaclPlanningStrategy::planSupportsShapeValidation(
    const QueryPlanner::SubtreePlan& plan, const std::string& shapeId) const {
  const auto* shape = shapeRegistry_->getShape(shapeId);
  if (shape == nullptr) {
    return false;
  }

  // Check if plan binds variables needed for validation
  // For a complete implementation, we'd analyze the plan's variable columns
  // and ensure all required properties are available

  // Simple check: plan must produce at least one column (the resource)
  return plan.getSizeEstimate() > 0;
}

// _____________________________________________________________________________
std::vector<QueryPlanner::SubtreePlan>
ShaclPlanningStrategy::reorderValidationOperations(
    std::vector<QueryPlanner::SubtreePlan> validationPlans) const {
  // Sort validation plans by cost (cheapest first)
  std::sort(validationPlans.begin(), validationPlans.end(),
            [](const QueryPlanner::SubtreePlan& a,
               const QueryPlanner::SubtreePlan& b) {
              return a.getCostEstimate() < b.getCostEstimate();
            });

  return validationPlans;
}

// _____________________________________________________________________________
std::unordered_map<std::string, ColumnIndex>
ShaclPlanningStrategy::createPropertyColumnMapping(
    const QueryPlanner::SubtreePlan& plan, const std::string& shapeId) const {
  std::unordered_map<std::string, ColumnIndex> mapping;

  const auto* shape = shapeRegistry_->getShape(shapeId);
  if (shape == nullptr) {
    return mapping;
  }

  // Get variable to column mapping from the plan
  const auto& varColumns = plan._qet->getVariableColumns();

  // Map each property shape to its column (if available)
  ColumnIndex colIdx = 1;  // Start after resource column (0)
  for (const auto& propShape : shape->propertyShapes) {
    // In a full implementation, we would look up the variable name
    // corresponding to this property path and find its column index
    // For now, use sequential column indices
    mapping[propShape.path] = colIdx++;
  }

  return mapping;
}

// _____________________________________________________________________________
size_t ShaclPlanningStrategy::estimateConstraintComplexity(
    const ShaclConstraint& constraint) const {
  // Estimate computational complexity of different constraint types
  switch (constraint.type) {
    case ConstraintType::MinCount:
    case ConstraintType::MaxCount:
      return 1;  // Simple counting

    case ConstraintType::NodeKind:
    case ConstraintType::Datatype:
      return 2;  // Type checking

    case ConstraintType::MinInclusive:
    case ConstraintType::MaxInclusive:
    case ConstraintType::MinLength:
    case ConstraintType::MaxLength:
      return 3;  // Comparisons

    case ConstraintType::Pattern:
      return 10;  // Regex matching is expensive

    case ConstraintType::In:
      return 5;  // Set membership check

    case ConstraintType::Unique:
    case ConstraintType::DisjointWith:
      return 15;  // Requires checking multiple values

    case ConstraintType::ClosedShape:
      return 8;  // Must check all properties

    case ConstraintType::Node:
    case ConstraintType::Shape:
      return 20;  // Recursive validation is most expensive

    default:
      return 5;  // Default complexity
  }
}

// _____________________________________________________________________________
bool ShaclPlanningStrategy::isConstraintPushable(
    const ShaclConstraint& constraint) const {
  // Determine which constraints can be pushed to index scans
  switch (constraint.type) {
    case ConstraintType::NodeKind:
      // Can filter by node type (IRI, Literal, BlankNode)
      return true;

    case ConstraintType::Datatype:
      // Can filter by datatype in index
      return true;

    case ConstraintType::MinCount:
    case ConstraintType::MaxCount:
      // Cardinality constraints can't be pushed to scans
      return false;

    case ConstraintType::Pattern:
      // Pattern matching might be pushable in some cases
      // (depends on index capabilities)
      return false;

    default:
      return false;
  }
}

// _____________________________________________________________________________
bool ShaclPlanningStrategy::hasShaclFilters(
    const std::vector<SparqlFilter>& filters) const {
  for (const auto& filter : filters) {
    // Check if filter expression contains SHACL predicates
    const auto& expr = filter.expression_;
    auto exprStr = expr->getDescriptor();

    // Look for SHACL-related terms
    if (exprStr.find(SHACL_NS) != std::string::npos ||
        exprStr.find("conforms") != std::string::npos ||
        exprStr.find("ValidationResult") != std::string::npos) {
      return true;
    }
  }

  return false;
}

// _____________________________________________________________________________
std::optional<std::string> ShaclPlanningStrategy::extractTargetShapeFromQuery(
    const parsedQuery::ParsedQuery& pq) const {
  // Look for explicit shape specification in the query
  // For example: BIND(<http://example.org/PersonShape> AS ?targetShape)

  for (const auto& child : pq._rootGraphPattern._graphPatterns) {
    if (std::holds_alternative<parsedQuery::Bind>(child)) {
      const auto& bind = std::get<parsedQuery::Bind>(child);
      // Check if binding involves a shape IRI
      // (would need to inspect the expression)

      // For now, check if target variable suggests a shape
      if (bind._target.name().find("shape") != std::string::npos ||
          bind._target.name().find("Shape") != std::string::npos) {
        // Try to extract IRI from expression
        // This is a simplified implementation
      }
    }
  }

  // No explicit shape found
  return std::nullopt;
}

}  // namespace shacl
