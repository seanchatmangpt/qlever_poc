#include "ShaclConstraintEvaluator.h"
#include "ShaclViolation.h"
#include <algorithm>
#include <cctype>

namespace shacl {

bool ShaclConstraintEvaluator::isBlankNode(const std::string& value) {
  return !value.empty() && value[0] == '_' && value.length() > 1 &&
         value[1] == ':';
}

bool ShaclConstraintEvaluator::isValidIri(const std::string& value) {
  if (value.empty()) return false;
  // Check if it looks like an IRI (contains :// or is in angle brackets)
  return (value.find("://") != std::string::npos ||
          (value.front() == '<' && value.back() == '>'));
}

bool ShaclConstraintEvaluator::isLiteral(const std::string& value) {
  if (value.empty()) return false;
  // Check if it's a quoted string or has a datatype annotation
  return (value.front() == '"') || (value.find("^^") != std::string::npos);
}

std::string ShaclConstraintEvaluator::extractLiteralValue(
    const std::string& rdfLiteral) {
  if (rdfLiteral.front() != '"') return rdfLiteral;

  auto endQuote = rdfLiteral.rfind('"');
  if (endQuote == std::string::npos || endQuote == 0) return rdfLiteral;

  return rdfLiteral.substr(1, endQuote - 1);
}

std::string ShaclConstraintEvaluator::extractLiteralDatatype(
    const std::string& rdfLiteral) {
  auto typeStart = rdfLiteral.find("^^");
  if (typeStart == std::string::npos) {
    // Check for language tag
    auto langStart = rdfLiteral.rfind('@');
    if (langStart != std::string::npos) {
      return "http://www.w3.org/1999/02/22-rdf-syntax-ns#langString";
    }
    return "http://www.w3.org/2001/XMLSchema#string";
  }

  return rdfLiteral.substr(typeStart + 2);
}

std::string ShaclConstraintEvaluator::getDatatype(const std::string& value) {
  if (isValidIri(value)) {
    return "http://www.w3.org/1999/02/22-rdf-syntax-ns#IRI";
  }
  if (isBlankNode(value)) {
    return "http://www.w3.org/1999/02/22-rdf-syntax-ns#BlankNode";
  }
  if (isLiteral(value)) {
    return extractLiteralDatatype(value);
  }
  return "http://www.w3.org/2001/XMLSchema#string";
}

bool ShaclConstraintEvaluator::matchesDatatype(const std::string& value,
                                               const std::string& datatype) {
  auto actualDatatype = getDatatype(value);
  return actualDatatype == datatype;
}

bool ShaclConstraintEvaluator::evaluateMinCount(
    int minCount, const std::vector<std::string>& values) {
  return static_cast<int>(values.size()) >= minCount;
}

bool ShaclConstraintEvaluator::evaluateMaxCount(
    int maxCount, const std::vector<std::string>& values) {
  return static_cast<int>(values.size()) <= maxCount;
}

bool ShaclConstraintEvaluator::evaluateMinLength(int minLength,
                                                 const std::string& value) {
  auto literalValue = extractLiteralValue(value);
  return static_cast<int>(literalValue.length()) >= minLength;
}

bool ShaclConstraintEvaluator::evaluateMaxLength(int maxLength,
                                                 const std::string& value) {
  auto literalValue = extractLiteralValue(value);
  return static_cast<int>(literalValue.length()) <= maxLength;
}

bool ShaclConstraintEvaluator::evaluatePattern(const std::string& pattern,
                                               const std::string& value) {
  try {
    auto literalValue = extractLiteralValue(value);
    std::regex regex(pattern);
    return std::regex_search(literalValue, regex);
  } catch (const std::regex_error&) {
    return false;
  }
}

bool ShaclConstraintEvaluator::evaluateInList(
    const std::vector<std::string>& allowedValues, const std::string& value) {
  return std::find(allowedValues.begin(), allowedValues.end(), value) !=
         allowedValues.end();
}

bool ShaclConstraintEvaluator::evaluateMinInclusive(const std::string& minValue,
                                                    const std::string& value) {
  try {
    auto min = std::stod(minValue);
    auto val = std::stod(extractLiteralValue(value));
    return val >= min;
  } catch (...) {
    return false;
  }
}

bool ShaclConstraintEvaluator::evaluateMaxInclusive(const std::string& maxValue,
                                                    const std::string& value) {
  try {
    auto max = std::stod(maxValue);
    auto val = std::stod(extractLiteralValue(value));
    return val <= max;
  } catch (...) {
    return false;
  }
}

bool ShaclConstraintEvaluator::evaluateNodeKind(NodeKind kind,
                                                const std::string& value) {
  switch (kind) {
    case NodeKind::IRI:
      return isValidIri(value);
    case NodeKind::BlankNode:
      return isBlankNode(value);
    case NodeKind::Literal:
      return isLiteral(value);
    case NodeKind::BlankNodeOrIRI:
      return isBlankNode(value) || isValidIri(value);
    case NodeKind::BlankNodeOrLiteral:
      return isBlankNode(value) || isLiteral(value);
    case NodeKind::IRIOrLiteral:
      return isValidIri(value) || isLiteral(value);
  }
  return false;
}

double ShaclConstraintEvaluator::parseNumericValue(const std::string& value) {
  try {
    return std::stod(extractLiteralValue(value));
  } catch (...) {
    return 0.0;
  }
}

bool ShaclConstraintEvaluator::evaluateUnique(
    const UniqueConstraintValue& constraint,
    const std::vector<std::string>& values,
    AdvancedConstraintContext* context) {
  if (!context || !context->stateTracker) {
    return true;  // Cannot validate without context
  }

  // Check each value for uniqueness across all resources
  for (const auto& value : values) {
    if (!context->stateTracker->trackUniqueValue(constraint.propertyPath,
                                                  value)) {
      return false;  // Duplicate found
    }
  }
  return true;
}

bool ShaclConstraintEvaluator::evaluateDisjointWith(
    const DisjointWithConstraintValue& constraint,
    const std::vector<std::string>& values1,
    const std::vector<std::string>& values2) {
  // Properties are disjoint if they have no overlapping values
  return !ResourceStateTracker().hasDisjointViolation(
      constraint.property1, constraint.property2, values1, values2);
}

bool ShaclConstraintEvaluator::evaluateClosed(
    const ClosedConstraintValue& constraint,
    const std::unordered_set<std::string>& properties) {
  if (!constraint.closed) {
    return true;
  }

  // Check if all properties are either allowed or ignored
  for (const auto& prop : properties) {
    bool isAllowed = std::find(constraint.allowedProperties.begin(),
                               constraint.allowedProperties.end(),
                               prop) != constraint.allowedProperties.end();
    bool isIgnored = std::find(constraint.ignoredProperties.begin(),
                               constraint.ignoredProperties.end(),
                               prop) != constraint.ignoredProperties.end();

    if (!isAllowed && !isIgnored) {
      return false;  // Unexpected property found
    }
  }
  return true;
}

bool ShaclConstraintEvaluator::evaluateHasValue(
    const HasValueConstraintValue& constraint,
    const std::vector<std::string>& values) {
  // Check if the required value exists in the list
  return std::find(values.begin(), values.end(), constraint.requiredValue) !=
         values.end();
}

bool ShaclConstraintEvaluator::evaluateMinExclusive(
    const MinExclusiveConstraintValue& constraint, const std::string& value) {
  try {
    double numValue = parseNumericValue(value);
    return numValue > constraint.minValue;
  } catch (...) {
    return false;
  }
}

bool ShaclConstraintEvaluator::evaluateMaxExclusive(
    const MaxExclusiveConstraintValue& constraint, const std::string& value) {
  try {
    double numValue = parseNumericValue(value);
    return numValue < constraint.maxValue;
  } catch (...) {
    return false;
  }
}

bool ShaclConstraintEvaluator::evaluateConstraint(
    const ShaclConstraint& constraint, const std::string& value,
    const std::string& datatype) {
  switch (constraint.type) {
    case ConstraintType::Datatype: {
      auto expectedDatatype = std::get<std::string>(constraint.value);
      return matchesDatatype(value, expectedDatatype);
    }
    case ConstraintType::NodeKind: {
      auto kind = std::get<NodeKind>(constraint.value);
      return evaluateNodeKind(kind, value);
    }
    case ConstraintType::Pattern: {
      auto pattern = std::get<std::string>(constraint.value);
      return evaluatePattern(pattern, value);
    }
    case ConstraintType::MinLength: {
      auto minLength = std::get<int>(constraint.value);
      return evaluateMinLength(minLength, value);
    }
    case ConstraintType::MaxLength: {
      auto maxLength = std::get<int>(constraint.value);
      return evaluateMaxLength(maxLength, value);
    }
    case ConstraintType::MinInclusive: {
      auto minValue = std::get<std::string>(constraint.value);
      return evaluateMinInclusive(minValue, value);
    }
    case ConstraintType::MaxInclusive: {
      auto maxValue = std::get<std::string>(constraint.value);
      return evaluateMaxInclusive(maxValue, value);
    }
    case ConstraintType::In: {
      auto allowedValues = std::get<std::vector<std::string>>(constraint.value);
      return evaluateInList(allowedValues, value);
    }
    case ConstraintType::MinExclusive: {
      auto minExclusiveConstraint =
          std::get<MinExclusiveConstraintValue>(constraint.value);
      return evaluateMinExclusive(minExclusiveConstraint, value);
    }
    case ConstraintType::MaxExclusive: {
      auto maxExclusiveConstraint =
          std::get<MaxExclusiveConstraintValue>(constraint.value);
      return evaluateMaxExclusive(maxExclusiveConstraint, value);
    }
    case ConstraintType::MinCount:
    case ConstraintType::MaxCount:
    case ConstraintType::Unique:
    case ConstraintType::DisjointWith:
    case ConstraintType::ClosedShape:
    case ConstraintType::HasValue:
      // These are handled at property/node level, not value level
      return true;
    case ConstraintType::Node:
    case ConstraintType::Shape:
      // Recursive constraints - handled by RecursiveShapeValidator
      // Should not be evaluated at value level
      return true;
    case ConstraintType::Sparql:
      // SPARQL constraints are handled at property/node level with context
      // Should not be evaluated at value level
      return true;
  }
  return true;
}

ValidationResult ShaclConstraintEvaluator::evaluatePropertyShape(
    const std::string& nodeId, const PropertyShape& propShape,
    const std::vector<std::string>& values) {
  ValidationResult result;
  result.focusNode = nodeId;

  // Check all constraints
  for (const auto& constraint : propShape.constraints) {
    bool satisfied = false;

    switch (constraint.type) {
      case ConstraintType::MinCount: {
        auto minCount = std::get<int>(constraint.value);
        satisfied = evaluateMinCount(minCount, values);
        break;
      }
      case ConstraintType::MaxCount: {
        auto maxCount = std::get<int>(constraint.value);
        satisfied = evaluateMaxCount(maxCount, values);
        break;
      }
      default: {
        // Check constraint against each value
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateConstraint(constraint, value)) {
            satisfied = false;
            break;
          }
        }
      }
    }

    if (!satisfied) {
      result.addViolation(constraint.message);
    }
  }

  // Check if required property is present
  if (propShape.required && values.empty()) {
    result.addViolation("Required property " + propShape.path +
                        " is missing");
  }

  return result;
}

ValidationResult ShaclConstraintEvaluator::evaluatePropertyShapeWithContext(
    const std::string& nodeId, const PropertyShape& propShape,
    const std::vector<std::string>& values,
    AdvancedConstraintContext* context) {
  ValidationResult result;
  result.focusNode = nodeId;

  // Check all constraints
  for (const auto& constraint : propShape.constraints) {
    bool satisfied = false;

    switch (constraint.type) {
      case ConstraintType::MinCount: {
        auto minCount = std::get<int>(constraint.value);
        satisfied = evaluateMinCount(minCount, values);
        if (!satisfied && !constraint.message.empty()) {
          result.addViolation(constraint.message);
        }
        break;
      }
      case ConstraintType::MaxCount: {
        auto maxCount = std::get<int>(constraint.value);
        satisfied = evaluateMaxCount(maxCount, values);
        if (!satisfied && !constraint.message.empty()) {
          result.addViolation(constraint.message);
        }
        break;
      }
      case ConstraintType::Unique: {
        auto uniqueConstraint =
            std::get<UniqueConstraintValue>(constraint.value);
        satisfied = evaluateUnique(uniqueConstraint, values, context);
        if (!satisfied) {
          result.addViolation(uniqueConstraint.message);
        }
        break;
      }
      case ConstraintType::HasValue: {
        auto hasValueConstraint =
            std::get<HasValueConstraintValue>(constraint.value);
        satisfied = evaluateHasValue(hasValueConstraint, values);
        if (!satisfied) {
          result.addViolation(hasValueConstraint.message);
        }
        break;
      }
      default: {
        // Check constraint against each value
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateConstraint(constraint, value)) {
            satisfied = false;
            break;
          }
        }
        if (!satisfied && !constraint.message.empty()) {
          result.addViolation(constraint.message);
        }
      }
    }
  }

  // Check if required property is present
  if (propShape.required && values.empty()) {
    result.addViolation("Required property " + propShape.path +
                        " is missing");
  }

  return result;
}

bool ShaclConstraintEvaluator::evaluateSparqlConstraint(
    const std::string& sparqlQuery,
    const std::string& focusNode,
    const std::unordered_map<std::string, std::string>& bindings,
    QueryExecutionContext* context) {
  // Create a SPARQL-based constraint and evaluate it
  SparqlBasedConstraint constraint(sparqlQuery);
  constraint.validateQuery();

  if (!constraint.isValid()) {
    // If query is invalid, treat as violation
    return false;
  }

  // Evaluate and check if there are violations
  auto violations = constraint.evaluate(focusNode, bindings, context);
  return violations.empty();  // true if no violations (constraint satisfied)
}

std::vector<std::string> ShaclConstraintEvaluator::evaluateSparqlConstraintWithResult(
    const SparqlBasedConstraint& constraint,
    const std::string& focusNode,
    const std::unordered_map<std::string, std::string>& bindings,
    QueryExecutionContext* context) {
  // Delegate to the SparqlBasedConstraint's evaluate method
  return constraint.evaluate(focusNode, bindings, context);
}

// Detailed violation reporting methods
std::vector<ShaclViolation> ShaclConstraintEvaluator::evaluatePropertyShapeDetailed(
    const std::string& nodeId, const PropertyShape& propShape,
    const std::vector<std::string>& values, const std::string& shapeId) {
  return evaluatePropertyShapeDetailedWithContext(nodeId, propShape, values,
                                                   shapeId, nullptr);
}

std::vector<ShaclViolation> ShaclConstraintEvaluator::evaluatePropertyShapeDetailedWithContext(
    const std::string& nodeId, const PropertyShape& propShape,
    const std::vector<std::string>& values, const std::string& shapeId,
    AdvancedConstraintContext* context) {
  std::vector<ShaclViolation> violations;

  // Check all constraints
  for (const auto& constraint : propShape.constraints) {
    bool satisfied = false;

    switch (constraint.type) {
      case ConstraintType::MinCount: {
        auto minCount = std::get<int>(constraint.value);
        satisfied = evaluateMinCount(minCount, values);
        if (!satisfied) {
          violations.push_back(ViolationFactory::createMinCountViolation(
              nodeId, propShape.path, shapeId, minCount,
              static_cast<int>(values.size())));
        }
        break;
      }

      case ConstraintType::MaxCount: {
        auto maxCount = std::get<int>(constraint.value);
        satisfied = evaluateMaxCount(maxCount, values);
        if (!satisfied) {
          violations.push_back(ViolationFactory::createMaxCountViolation(
              nodeId, propShape.path, shapeId, maxCount,
              static_cast<int>(values.size())));
        }
        break;
      }

      case ConstraintType::Datatype: {
        auto expectedDatatype = std::get<std::string>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!matchesDatatype(value, expectedDatatype)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createDatatypeViolation(
                nodeId, propShape.path, shapeId, expectedDatatype, value));
          }
        }
        break;
      }

      case ConstraintType::NodeKind: {
        auto kind = std::get<NodeKind>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateNodeKind(kind, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createNodeKindViolation(
                nodeId, propShape.path, shapeId, kind, value));
          }
        }
        break;
      }

      case ConstraintType::Pattern: {
        auto pattern = std::get<std::string>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluatePattern(pattern, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createPatternViolation(
                nodeId, propShape.path, shapeId, pattern, value));
          }
        }
        break;
      }

      case ConstraintType::MinLength: {
        auto minLength = std::get<int>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateMinLength(minLength, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createMinLengthViolation(
                nodeId, propShape.path, shapeId, minLength, value));
          }
        }
        break;
      }

      case ConstraintType::MaxLength: {
        auto maxLength = std::get<int>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateMaxLength(maxLength, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createMaxLengthViolation(
                nodeId, propShape.path, shapeId, maxLength, value));
          }
        }
        break;
      }

      case ConstraintType::MinInclusive: {
        auto minValue = std::get<std::string>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateMinInclusive(minValue, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createMinInclusiveViolation(
                nodeId, propShape.path, shapeId, minValue, value));
          }
        }
        break;
      }

      case ConstraintType::MaxInclusive: {
        auto maxValue = std::get<std::string>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateMaxInclusive(maxValue, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createMaxInclusiveViolation(
                nodeId, propShape.path, shapeId, maxValue, value));
          }
        }
        break;
      }

      case ConstraintType::In: {
        auto allowedValues = std::get<std::vector<std::string>>(constraint.value);
        satisfied = true;
        for (const auto& value : values) {
          if (!evaluateInList(allowedValues, value)) {
            satisfied = false;
            violations.push_back(ViolationFactory::createInViolation(
                nodeId, propShape.path, shapeId, allowedValues, value));
          }
        }
        break;
      }

      case ConstraintType::Unique:
      case ConstraintType::DisjointWith:
      case ConstraintType::ClosedShape:
        // These are handled at property/node level, not value level
        // For advanced constraints, delegate to the appropriate evaluator
        if (context) {
          // Advanced constraint evaluation would go here
          // This would require access to AdvancedConstraints functionality
        }
        satisfied = true;
        break;
    }
  }

  // Check if required property is present
  if (propShape.required && values.empty()) {
    violations.push_back(ViolationFactory::createMinCountViolation(
        nodeId, propShape.path, shapeId, 1, 0));
  }

  return violations;
}

}  // namespace shacl
