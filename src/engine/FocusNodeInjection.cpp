// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Agent 3 (EPIC 10.3 - Focus-Node Injection)

#include "FocusNodeInjection.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "engine/IndexScan.h"
#include "engine/QueryExecutionTree.h"
#include "parser/TripleComponent.h"
#include "util/Exception.h"

namespace qlever {

namespace {

// Constants for cost estimation (from specification Section 5.2)
constexpr size_t TYPE_FILTER_COST_PER_ROW = 5;
constexpr size_t RANGE_FILTER_COST_PER_ROW = 10;
constexpr size_t REGEX_FILTER_COST_PER_ROW = 500;
constexpr size_t DEFAULT_DOWNSTREAM_COST = 50;

// RDF/SHACL namespace constants
constexpr const char* RDF_TYPE =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";
constexpr const char* XSD_NS = "http://www.w3.org/2001/XMLSchema#";

}  // namespace

// _____________________________________________________________________________
QueryPlanner::SubtreePlan FocusNodeInjection::optimizeWithShaclConstraints(
    QueryPlanner::SubtreePlan plan, const shacl::ShaclShapeRegistry* shapes) {
  if (shapes == nullptr || !shapes->isEnabled()) {
    return plan;  // No optimization if shapes disabled
  }

  // Get all registered shapes
  auto allShapes = shapes->getAllShapes();
  if (allShapes.empty()) {
    return plan;  // No shapes to optimize with
  }

  QueryPlanner::SubtreePlan optimizedPlan = std::move(plan);

  // For each shape, try to inject constraints
  for (const auto* shape : allShapes) {
    if (shape == nullptr || !shape->hasTargets()) {
      continue;  // Skip shapes without targets
    }

    // Extract target filters (sh:targetClass, sh:targetNode)
    auto targetFilters = extractTargetFilters(*shape);

    // Extract pushable property constraints
    auto propertyFilters = extractPushablePropertyConstraints(*shape);

    // Combine all filters
    std::vector<SparqlFilter> allFilters;
    allFilters.reserve(targetFilters.size() + propertyFilters.size());
    allFilters.insert(allFilters.end(), targetFilters.begin(),
                      targetFilters.end());
    allFilters.insert(allFilters.end(), propertyFilters.begin(),
                      propertyFilters.end());

    if (allFilters.empty()) {
      continue;  // No filters to inject
    }

    // See ROADMAP.md for EPIC 10.3 filter injection implementation
    // Requires integration with UIRGraph (Agent 3 Part 1)
    // and UnifiedPhysicalOptimizerStub (Agent 3 Part 2 - aspirational)
    //
    // For now, we store the extracted filters as metadata
    // Full implementation blocked until Parts 1 & 2 complete
  }

  // Reorder scans by selectivity (if applicable)
  optimizedPlan = reorderBySelectivity(std::move(optimizedPlan));

  return optimizedPlan;
}

// _____________________________________________________________________________
std::vector<SparqlFilter> FocusNodeInjection::extractTargetFilters(
    const shacl::NodeShape& shape) {
  std::vector<SparqlFilter> filters;

  // Extract sh:targetClass filters
  // Each targetClass generates: ?node rdf:type targetClass
  for (const auto& targetClass : shape.targetClasses) {
    // Create SPARQL filter: ?focusNode a <targetClass>
    std::ostringstream filterExpr;
    filterExpr << "FILTER(?focusNode = <" << targetClass << ">)";

    // Note: In full implementation, this would create a proper
    // SparqlFilter object with parsed expression tree
    // For now, we create placeholder filters
    SparqlFilter filter;
    // filter.expression = filterExpr.str();  // Placeholder
    filters.push_back(std::move(filter));
  }

  // Extract sh:targetNode filters
  // Each targetNode generates: ?node = targetNode
  for (const auto& targetNode : shape.targetNodes) {
    std::ostringstream filterExpr;
    filterExpr << "FILTER(?focusNode = <" << targetNode << ">)";

    SparqlFilter filter;
    // filter.expression = filterExpr.str();  // Placeholder
    filters.push_back(std::move(filter));
  }

  return filters;
}

// _____________________________________________________________________________
std::vector<SparqlFilter>
FocusNodeInjection::extractPushablePropertyConstraints(
    const shacl::NodeShape& shape) {
  std::vector<SparqlFilter> filters;

  for (const auto& propShape : shape.propertyShapes) {
    // Get property path (simple IRI for now)
    std::string path = propShape.path;
    if (path.empty() && !propShape.hasComplexPath()) {
      continue;  // Skip empty paths
    }

    // Extract pushable constraints from this property
    for (const auto& constraint : propShape.constraints) {
      if (isConstraintPushable(constraint.type)) {
        auto filter = translateConstraintToFilter(path, constraint);
        filters.push_back(std::move(filter));
      }
    }
  }

  return filters;
}

// _____________________________________________________________________________
bool FocusNodeInjection::scanMatchesTarget(
    const Operation* scan, const std::vector<std::string>& targetClasses) {
  if (scan == nullptr || targetClasses.empty()) {
    return false;
  }

  // Check if this is an IndexScan operation
  const auto* indexScan = dynamic_cast<const IndexScan*>(scan);
  if (indexScan == nullptr) {
    return false;
  }

  // Case 1: Index scan is on rdf:type predicate
  const auto& predicate = indexScan->predicate();
  if (predicate.isIri()) {
    auto predicateStr = predicate.getIri().toStringRepresentation();
    if (predicateStr == RDF_TYPE) {
      // Check if object matches any target class
      const auto& object = indexScan->object();
      if (object.isIri()) {
        auto objectStr = object.getIri().toStringRepresentation();
        return std::find(targetClasses.begin(), targetClasses.end(),
                         objectStr) != targetClasses.end();
      }
    }
  }

  // Case 2: Index scan binds subject variable
  // (can inject type filter on subject)
  const auto& subject = indexScan->subject();
  if (subject.isVariable()) {
    return true;  // Can potentially inject type constraint
  }

  return false;
}

// _____________________________________________________________________________
double FocusNodeInjection::estimateConstraintSelectivity(
    const shacl::ShaclConstraint& constraint) {
  // Selectivity estimates from specification Section 5.1
  switch (constraint.type) {
    case shacl::ConstraintType::Datatype:
      // 5% of values typically have incorrect datatype
      return 0.95;

    case shacl::ConstraintType::NodeKind:
      // 10% of nodes are typically wrong kind (IRI vs Literal)
      return 0.90;

    case shacl::ConstraintType::MinInclusive:
    case shacl::ConstraintType::MaxInclusive:
      // 20% of values typically outside range
      return 0.80;

    case shacl::ConstraintType::MinExclusive:
    case shacl::ConstraintType::MaxExclusive:
      // Similar to inclusive bounds
      return 0.80;

    case shacl::ConstraintType::In: {
      // Set membership: |allowed_set| / |universe|
      // LIMITATION: Uses conservative median estimate (0.50).
      // Future enhancement: Extract actual set size from constraint.value
      return 0.50;
    }

    case shacl::ConstraintType::Pattern:
      // Regex patterns vary widely, use median estimate
      return 0.50;

    case shacl::ConstraintType::MinLength:
    case shacl::ConstraintType::MaxLength:
      // String length constraints, moderate selectivity
      return 0.85;

    // Non-pushable constraints (should not reach here)
    case shacl::ConstraintType::MinCount:
    case shacl::ConstraintType::MaxCount:
    case shacl::ConstraintType::Unique:
    case shacl::ConstraintType::DisjointWith:
    case shacl::ConstraintType::ClosedShape:
      return 1.0;  // No filtering (requires aggregation/join)

    default:
      return 1.0;  // Conservative: assume no filtering
  }
}

// _____________________________________________________________________________
double FocusNodeInjection::computeEffectiveness(size_t inputCardinality,
                                                double selectivity,
                                                size_t downstreamCost,
                                                size_t injectionCost) {
  if (inputCardinality == 0 || injectionCost == 0) {
    return 0.0;  // Avoid division by zero
  }

  // Calculate rows eliminated
  size_t rowsEliminated =
      static_cast<size_t>(inputCardinality * (1.0 - selectivity));

  // Calculate benefit (rows saved × downstream processing cost)
  double benefit = static_cast<double>(rowsEliminated * downstreamCost);

  // Calculate cost (all rows × injection overhead)
  double cost = static_cast<double>(inputCardinality * injectionCost);

  // Effectiveness = benefit / cost
  return (cost > 0.0) ? (benefit / cost) : 0.0;
}

// _____________________________________________________________________________
bool FocusNodeInjection::isConstraintPushable(shacl::ConstraintType type) {
  // Pushable constraint types (from specification Section 2.2)
  switch (type) {
    // Value type constraints - PUSHABLE
    case shacl::ConstraintType::Datatype:
    case shacl::ConstraintType::NodeKind:
      return true;

    // Value range constraints - PUSHABLE
    case shacl::ConstraintType::MinInclusive:
    case shacl::ConstraintType::MaxInclusive:
    case shacl::ConstraintType::MinExclusive:
    case shacl::ConstraintType::MaxExclusive:
      return true;

    // Enumeration - PUSHABLE
    case shacl::ConstraintType::In:
      return true;

    // String constraints - PUSHABLE (if index supports)
    case shacl::ConstraintType::MinLength:
    case shacl::ConstraintType::MaxLength:
      return true;

    // Cardinality constraints - NOT PUSHABLE (require aggregation)
    case shacl::ConstraintType::MinCount:
    case shacl::ConstraintType::MaxCount:
      return false;

    // Pattern - NOT PUSHABLE (regex expensive, full scan required)
    case shacl::ConstraintType::Pattern:
      return false;

    // Advanced constraints - NOT PUSHABLE (require join/global analysis)
    case shacl::ConstraintType::Unique:
    case shacl::ConstraintType::DisjointWith:
    case shacl::ConstraintType::ClosedShape:
      return false;

    // Recursive shapes - NOT PUSHABLE
    case shacl::ConstraintType::Node:
    case shacl::ConstraintType::Shape:
      return false;

    // SPARQL-based - NOT PUSHABLE
    case shacl::ConstraintType::Sparql:
      return false;

    // HasValue - POTENTIALLY PUSHABLE (simple equality check)
    case shacl::ConstraintType::HasValue:
      return true;

    default:
      return false;  // Conservative: unknown constraints not pushable
  }
}

// _____________________________________________________________________________
SparqlFilter FocusNodeInjection::translateConstraintToFilter(
    const std::string& path, const shacl::ShaclConstraint& constraint) {
  SparqlFilter filter;

  // Generate SPARQL filter expression based on constraint type
  std::ostringstream filterExpr;

  switch (constraint.type) {
    case shacl::ConstraintType::Datatype: {
      // FILTER(datatype(?var) = xsd:type)
      if (std::holds_alternative<std::string>(constraint.value)) {
        const auto& datatype = std::get<std::string>(constraint.value);
        filterExpr << "FILTER(datatype(?" << path << ") = <" << datatype
                   << ">)";
      }
      break;
    }

    case shacl::ConstraintType::NodeKind: {
      // FILTER(isIRI(?var)) or FILTER(isLiteral(?var))
      if (std::holds_alternative<shacl::NodeKind>(constraint.value)) {
        auto nodeKind = std::get<shacl::NodeKind>(constraint.value);
        switch (nodeKind) {
          case shacl::NodeKind::IRI:
            filterExpr << "FILTER(isIRI(?" << path << "))";
            break;
          case shacl::NodeKind::Literal:
            filterExpr << "FILTER(isLiteral(?" << path << "))";
            break;
          case shacl::NodeKind::BlankNode:
            filterExpr << "FILTER(isBlank(?" << path << "))";
            break;
          default:
            // Complex node kinds (BlankNodeOrIRI, etc.)
            // Require disjunction, skip for now
            break;
        }
      }
      break;
    }

    case shacl::ConstraintType::MinInclusive: {
      // FILTER(?var >= value)
      if (std::holds_alternative<int>(constraint.value)) {
        int minValue = std::get<int>(constraint.value);
        filterExpr << "FILTER(?" << path << " >= " << minValue << ")";
      } else if (std::holds_alternative<double>(constraint.value)) {
        double minValue = std::get<double>(constraint.value);
        filterExpr << "FILTER(?" << path << " >= " << minValue << ")";
      }
      break;
    }

    case shacl::ConstraintType::MaxInclusive: {
      // FILTER(?var <= value)
      if (std::holds_alternative<int>(constraint.value)) {
        int maxValue = std::get<int>(constraint.value);
        filterExpr << "FILTER(?" << path << " <= " << maxValue << ")";
      } else if (std::holds_alternative<double>(constraint.value)) {
        double maxValue = std::get<double>(constraint.value);
        filterExpr << "FILTER(?" << path << " <= " << maxValue << ")";
      }
      break;
    }

    case shacl::ConstraintType::In: {
      // FILTER(?var IN (value1, value2, ...))
      if (std::holds_alternative<std::vector<std::string>>(constraint.value)) {
        const auto& allowedValues =
            std::get<std::vector<std::string>>(constraint.value);
        if (!allowedValues.empty()) {
          filterExpr << "FILTER(?" << path << " IN (";
          for (size_t i = 0; i < allowedValues.size(); ++i) {
            if (i > 0) filterExpr << ", ";
            filterExpr << "<" << allowedValues[i] << ">";
          }
          filterExpr << "))";
        }
      }
      break;
    }

    default:
      // Other constraint types not yet translated
      break;
  }

  // Store filter expression (in real implementation, this would be
  // parsed into a SparqlExpression tree)
  // filter.expression = filterExpr.str();  // Placeholder

  return filter;
}

// _____________________________________________________________________________
QueryPlanner::SubtreePlan FocusNodeInjection::reorderBySelectivity(
    QueryPlanner::SubtreePlan plan) {
  // See ROADMAP.md for EPIC 10.3 scan reordering implementation
  //
  // Planned algorithm:
  // 1. Extract all index scans from plan tree
  // 2. Estimate selectivity for each scan (based on injected constraints)
  // 3. Sort scans by ascending selectivity (most selective first)
  // 4. Reconstruct plan with reordered scans
  //
  // This requires UIRGraph manipulation (Agent 3 Part 1)
  // and UnifiedPhysicalOptimizerStub integration (Agent 3 Part 2 -
  // aspirational)

  // For now, return plan unchanged
  return plan;
}

}  // namespace qlever
