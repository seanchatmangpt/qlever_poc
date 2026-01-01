#include "ShaclConstraintEvaluator.h"
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
    case ConstraintType::MinCount:
    case ConstraintType::MaxCount:
    case ConstraintType::Unique:
    case ConstraintType::DisjointWith:
    case ConstraintType::ClosedShape:
      // These are handled at property/node level, not value level
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

}  // namespace shacl
