#include "ShExValidator.h"
#include <algorithm>
#include <sstream>

namespace shex {

// Load shapes from ShapeMap
void ShExValidator::loadShapes(const ShapeMap& shapeMap) {
  shapeMap_ = shapeMap;
  resetStats();
}

// Validate a single node against a shape
ValidationResult ShExValidator::validateNode(
    const std::string& nodeId, const std::string& shapeId,
    const RDFGraph& graph, const ValidationConfig& config) {
  ValidationResult result(nodeId, shapeId);
  stats_.nodesValidated++;

  // Check if we've exceeded node limit
  if (config.failClosed && stats_.nodesValidated > config.maxFocusNodes) {
    result.addViolation(
        "Exceeded maximum focus nodes limit: " +
        std::to_string(config.maxFocusNodes));
    stats_.limitExceeded = true;
    return result;
  }

  // Get the shape
  auto shape = shapeMap_.getShape(shapeId);
  if (!shape) {
    result.addViolation("Shape not found: " + shapeId);
    return result;
  }

  // Validate node against shape
  validateNodeInternal(nodeId, shape, graph, result, config);

  if (!result.conforms) {
    stats_.violationsFound += result.violations.size();
  }

  return result;
}

// Validate multiple nodes against a shape
ValidationReport ShExValidator::validateNodes(
    const std::vector<std::string>& nodeIds, const std::string& shapeId,
    const RDFGraph& graph, const ValidationConfig& config) {
  ValidationReport report;

  for (const auto& nodeId : nodeIds) {
    auto result = validateNode(nodeId, shapeId, graph, config);
    report.addResult(result);

    // Check if we should stop (fail-closed)
    if (config.failClosed &&
        report.violationCount >= config.maxViolations) {
      ValidationResult limitResult("", shapeId);
      limitResult.addViolation(
          "Exceeded maximum violations limit: " +
          std::to_string(config.maxViolations));
      report.addResult(limitResult);
      stats_.limitExceeded = true;
      break;
    }
  }

  return report;
}

// Validate node against shape (internal)
bool ShExValidator::validateNodeInternal(
    const std::string& nodeId, const std::shared_ptr<ShapeExpr>& shape,
    const RDFGraph& graph, ValidationResult& result,
    const ValidationConfig& config) {
  bool allValid = true;

  // Validate each triple constraint
  for (const auto& tc : shape->tripleConstraints) {
    if (!validateTripleConstraint(nodeId, tc, graph, result, config)) {
      allValid = false;
      if (!config.collectAllViolations) {
        break;  // Stop at first violation
      }
    }

    // Check constraint evaluation limit
    stats_.constraintsEvaluated++;
    if (config.failClosed &&
        stats_.constraintsEvaluated > config.maxConstraintEvals) {
      result.addViolation(
          "Exceeded maximum constraint evaluations: " +
          std::to_string(config.maxConstraintEvals));
      stats_.limitExceeded = true;
      return false;
    }
  }

  // Validate CLOSED constraint
  if (shape->closed) {
    if (!validateClosed(nodeId, shape, graph, result)) {
      allValid = false;
    }
  }

  return allValid;
}

// Validate triple constraint
bool ShExValidator::validateTripleConstraint(
    const std::string& nodeId, const TripleConstraint& tc,
    const RDFGraph& graph, ValidationResult& result,
    const ValidationConfig& config) {
  // Get all objects for this predicate
  auto objects = graph.getObjects(nodeId, tc.predicate);
  size_t actualCount = objects.size();

  // Validate cardinality
  if (!validateCardinality(nodeId, tc, actualCount, result)) {
    return false;
  }

  // Validate each value
  for (const auto& object : objects) {
    bool valueValid = true;

    // Validate datatype if specified
    if (tc.valueType.has_value()) {
      if (!validateDatatype(object, tc.valueType.value(), result,
                             tc.predicate)) {
        valueValid = false;
      }
    }

    // Validate node kind if specified
    if (tc.nodeKind.has_value()) {
      if (!validateNodeKind(object, tc.nodeKind.value(), result,
                             tc.predicate)) {
        valueValid = false;
      }
    }

    // Validate nested shape if specified
    if (tc.hasNestedShape() && tc.valueExpr.has_value()) {
      ValidationResult nestedResult(object, tc.valueExpr.value()->shapeId);
      if (!validateNodeInternal(object, tc.valueExpr.value(), graph,
                                 nestedResult, config)) {
        result.addViolation("Nested shape validation failed for " + object +
                             " at property " + tc.predicate);
        valueValid = false;
      }
    }

    if (!valueValid && !config.collectAllViolations) {
      return false;
    }
  }

  return true;
}

// Validate cardinality
bool ShExValidator::validateCardinality(const std::string& nodeId,
                                         const TripleConstraint& tc,
                                         size_t actualCount,
                                         ValidationResult& result) {
  int minCount = tc.getMinCount();
  int maxCount = tc.getMaxCount();

  // Check minimum
  if (actualCount < static_cast<size_t>(minCount)) {
    std::ostringstream msg;
    msg << "Cardinality violation at " << tc.predicate << ": expected at least "
        << minCount << " value(s), found " << actualCount;
    result.addViolation(msg.str());
    return false;
  }

  // Check maximum (if not unbounded)
  if (maxCount >= 0 && actualCount > static_cast<size_t>(maxCount)) {
    std::ostringstream msg;
    msg << "Cardinality violation at " << tc.predicate << ": expected at most "
        << maxCount << " value(s), found " << actualCount;
    result.addViolation(msg.str());
    return false;
  }

  return true;
}

// Validate datatype
bool ShExValidator::validateDatatype(const std::string& value,
                                       const ValueType& expectedType,
                                       ValidationResult& result,
                                       const std::string& propertyPath) {
  // Get actual datatype from literal
  std::string actualType = RDFGraph::getLiteralDatatype(value);

  if (actualType.empty()) {
    // No datatype specified - assume plain literal
    // Check if expected type is xsd:string
    if (expectedType.datatype.find("string") == std::string::npos) {
      std::ostringstream msg;
      msg << "Datatype violation at " << propertyPath
          << ": expected " << expectedType.datatype
          << ", found plain literal";
      result.addViolation(msg.str());
      return false;
    }
    return true;
  }

  // Compare datatypes
  if (actualType != expectedType.datatype &&
      actualType.find(expectedType.datatype) == std::string::npos) {
    std::ostringstream msg;
    msg << "Datatype violation at " << propertyPath << ": expected "
        << expectedType.datatype << ", found " << actualType;
    result.addViolation(msg.str());
    return false;
  }

  return true;
}

// Validate node kind
bool ShExValidator::validateNodeKind(const std::string& value,
                                       ShExNodeKind expectedKind,
                                       ValidationResult& result,
                                       const std::string& propertyPath) {
  bool isIRI = RDFGraph::isIRI(value);
  bool isLiteral = RDFGraph::isLiteral(value);
  bool isBlankNode = RDFGraph::isBlankNode(value);

  bool valid = false;
  std::string expectedKindStr;

  switch (expectedKind) {
    case ShExNodeKind::IRI:
      valid = isIRI;
      expectedKindStr = "IRI";
      break;
    case ShExNodeKind::BlankNode:
      valid = isBlankNode;
      expectedKindStr = "BlankNode";
      break;
    case ShExNodeKind::Literal:
      valid = isLiteral;
      expectedKindStr = "Literal";
      break;
    case ShExNodeKind::NonLiteral:
      valid = isIRI || isBlankNode;
      expectedKindStr = "NonLiteral (IRI or BlankNode)";
      break;
  }

  if (!valid) {
    std::ostringstream msg;
    msg << "Node kind violation at " << propertyPath << ": expected "
        << expectedKindStr;
    result.addViolation(msg.str());
    return false;
  }

  return true;
}

// Validate CLOSED constraint
bool ShExValidator::validateClosed(const std::string& nodeId,
                                     const std::shared_ptr<ShapeExpr>& shape,
                                     const RDFGraph& graph,
                                     ValidationResult& result) {
  // Get all properties of the node
  auto properties = graph.getProperties(nodeId);

  // Check each property
  for (const auto& prop : properties) {
    if (!shape->isPropertyAllowed(prop)) {
      std::ostringstream msg;
      msg << "CLOSED shape violation: property " << prop
          << " not allowed in shape " << shape->shapeId;
      result.addViolation(msg.str());
      return false;
    }
  }

  return true;
}

// Check validation limits
bool ShExValidator::checkLimits(const ValidationConfig& config,
                                 ValidationResult& result) {
  if (config.failClosed) {
    if (stats_.nodesValidated > config.maxFocusNodes) {
      result.addViolation("Exceeded maximum focus nodes");
      return false;
    }
    if (stats_.constraintsEvaluated > config.maxConstraintEvals) {
      result.addViolation("Exceeded maximum constraint evaluations");
      return false;
    }
    if (stats_.violationsFound > config.maxViolations) {
      result.addViolation("Exceeded maximum violations");
      return false;
    }
  }
  return true;
}

// Format violations
std::string ShExValidator::formatViolations(
    const ValidationResult& result) const {
  std::ostringstream out;

  out << "Node: " << result.focusNode << "\n";
  out << "Shape: " << result.shapeId << "\n";
  out << "Conforms: " << (result.conforms ? "Yes" : "No") << "\n";

  if (!result.violations.empty()) {
    out << "Violations:\n";
    for (const auto& violation : result.violations) {
      out << "  - " << violation << "\n";
    }
  }

  return out.str();
}

// Format report
std::string ShExValidator::formatReport(
    const ValidationReport& report) const {
  std::ostringstream out;

  out << "=== ShEx Validation Report ===\n";
  out << "Conforms: " << (report.conforms ? "Yes" : "No") << "\n";
  out << "Total Results: " << report.results.size() << "\n";
  out << "Conforming: " << report.conformingCount() << "\n";
  out << "Non-Conforming: " << report.nonConformingCount() << "\n";
  out << "Total Violations: " << report.violationCount << "\n";
  out << "\n";

  // Print each result
  for (const auto& result : report.results) {
    if (!result.conforms) {
      out << formatViolations(result) << "\n";
    }
  }

  return out.str();
}

// Get loaded shape IDs
std::vector<std::string> ShExValidator::getShapeIds() const {
  return shapeMap_.getShapeIds();
}

// Check if a shape exists
bool ShExValidator::hasShape(const std::string& shapeId) const {
  return shapeMap_.hasShape(shapeId);
}

// Reset statistics
void ShExValidator::resetStats() {
  stats_ = ValidationStats();
}

}  // namespace shex
