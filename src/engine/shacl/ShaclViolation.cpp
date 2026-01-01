#include "ShaclViolation.h"
#include <sstream>
#include <algorithm>

namespace shacl {

// Empty vector for returning when no violations found
static const std::vector<ShaclViolation> emptyViolations;

// ShaclViolation implementations
std::string ShaclViolation::getOrGenerateMessage() const {
  if (!message.empty()) {
    return message;
  }

  // Generate a default message based on the constraint type
  std::ostringstream oss;
  oss << "Validation failed for node <" << focusNode << ">";

  if (!resultPath.empty()) {
    oss << " on property <" << resultPath << ">";
  }

  oss << ": constraint " << sourceConstraintComponent << " violated";

  if (expectedValue) {
    oss << " (expected: " << *expectedValue << ")";
  }

  if (!values.empty()) {
    oss << " (actual: ";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << values[i];
    }
    oss << ")";
  }

  return oss.str();
}

std::string ShaclViolation::getSummary() const {
  std::ostringstream oss;
  oss << "[" << (severity == SeverityLevel::Violation   ? "ERROR"
                 : severity == SeverityLevel::Warning ? "WARNING"
                                                        : "INFO")
      << "] " << focusNode;

  if (!resultPath.empty()) {
    oss << " / " << resultPath;
  }

  oss << ": " << sourceConstraintComponent;

  return oss.str();
}

std::string ShaclViolation::getDetailedDescription() const {
  std::ostringstream oss;

  oss << "Violation Details:\n";
  oss << "  Focus Node: " << focusNode << "\n";

  if (!resultPath.empty()) {
    oss << "  Result Path: " << resultPath << "\n";
  }

  oss << "  Source Shape: " << sourceShape << "\n";
  oss << "  Constraint: " << sourceConstraintComponent << "\n";
  oss << "  Severity: "
      << (severity == SeverityLevel::Violation   ? "Violation"
          : severity == SeverityLevel::Warning ? "Warning"
                                                 : "Info")
      << "\n";

  if (!message.empty()) {
    oss << "  Message: " << message << "\n";
  }

  if (expectedValue) {
    oss << "  Expected: " << *expectedValue << "\n";
  }

  if (!values.empty()) {
    oss << "  Actual Value(s): ";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << values[i];
    }
    oss << "\n";
  }

  if (!details.empty()) {
    oss << "  Additional Details:\n";
    for (const auto& [key, value] : details) {
      oss << "    " << key << ": " << value << "\n";
    }
  }

  return oss.str();
}

// DetailedValidationReport implementations
void DetailedValidationReport::addViolation(const ShaclViolation& violation) {
  violations.push_back(violation);
  updateIndices(violation);
}

void DetailedValidationReport::addViolation(ShaclViolation&& violation) {
  updateIndices(violation);
  violations.push_back(std::move(violation));
}

void DetailedValidationReport::updateIndices(const ShaclViolation& violation) {
  // Update overall conformance
  if (violation.isError()) {
    conforms = false;
    totalViolations++;
  } else if (violation.isWarning()) {
    totalWarnings++;
  } else {
    totalInfo++;
  }

  // Index by focus node
  violationsByFocusNode[violation.focusNode].push_back(violation);

  // Index by shape
  violationsByShape[violation.sourceShape].push_back(violation);

  // Index by constraint
  violationsByConstraint[violation.sourceConstraintComponent].push_back(
      violation);

  // Index by severity
  violationsBySeverity[violation.severity].push_back(violation);
}

const std::vector<ShaclViolation>&
DetailedValidationReport::getViolationsForNode(
    const std::string& focusNode) const {
  auto it = violationsByFocusNode.find(focusNode);
  if (it != violationsByFocusNode.end()) {
    return it->second;
  }
  return emptyViolations;
}

const std::vector<ShaclViolation>&
DetailedValidationReport::getViolationsForShape(
    const std::string& shapeId) const {
  auto it = violationsByShape.find(shapeId);
  if (it != violationsByShape.end()) {
    return it->second;
  }
  return emptyViolations;
}

std::vector<ShaclViolation> DetailedValidationReport::getViolationsBySeverity(
    SeverityLevel severity) const {
  auto it = violationsBySeverity.find(severity);
  if (it != violationsBySeverity.end()) {
    return it->second;
  }
  return {};
}

DetailedValidationReport::Summary DetailedValidationReport::getSummary() const {
  Summary summary;

  // Count unique focus nodes
  summary.totalResources = violationsByFocusNode.size();

  // Count violations by shape
  for (const auto& [shapeId, viols] : violationsByShape) {
    summary.violationsByShape[shapeId] = viols.size();
  }

  // Count violations by constraint
  for (const auto& [constraint, viols] : violationsByConstraint) {
    summary.violationsByConstraint[constraint] = viols.size();
  }

  // Set severity counts
  summary.violations = totalViolations;
  summary.warnings = totalWarnings;
  summary.infos = totalInfo;

  // Calculate conforming vs non-conforming
  size_t nonConformingNodes = 0;
  for (const auto& [node, viols] : violationsByFocusNode) {
    bool hasError =
        std::any_of(viols.begin(), viols.end(),
                    [](const ShaclViolation& v) { return v.isError(); });
    if (hasError) {
      nonConformingNodes++;
    }
  }

  summary.nonConformingResources = nonConformingNodes;
  summary.conformingResources = summary.totalResources - nonConformingNodes;

  return summary;
}

// ViolationFactory implementations
std::string ViolationFactory::nodeKindToString(NodeKind kind) {
  switch (kind) {
    case NodeKind::IRI:
      return "IRI";
    case NodeKind::BlankNode:
      return "BlankNode";
    case NodeKind::Literal:
      return "Literal";
    case NodeKind::BlankNodeOrIRI:
      return "BlankNode or IRI";
    case NodeKind::BlankNodeOrLiteral:
      return "BlankNode or Literal";
    case NodeKind::IRIOrLiteral:
      return "IRI or Literal";
  }
  return "Unknown";
}

ShaclViolation ViolationFactory::createMinCountViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, int expectedMin, int actualCount) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:minCount");

  std::ostringstream msg;
  msg << "Property " << path << " has " << actualCount
      << " value(s), but at least " << expectedMin << " required";
  violation.message = msg.str();

  violation.expectedValue = std::to_string(expectedMin);
  violation.withDetail("actualCount", std::to_string(actualCount));

  return violation;
}

ShaclViolation ViolationFactory::createMaxCountViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, int expectedMax, int actualCount) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:maxCount");

  std::ostringstream msg;
  msg << "Property " << path << " has " << actualCount
      << " value(s), but at most " << expectedMax << " allowed";
  violation.message = msg.str();

  violation.expectedValue = std::to_string(expectedMax);
  violation.withDetail("actualCount", std::to_string(actualCount));

  return violation;
}

ShaclViolation ViolationFactory::createDatatypeViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, const std::string& expectedType,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:datatype");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' does not have expected datatype <"
      << expectedType << ">";
  violation.message = msg.str();

  violation.expectedValue = expectedType;
  violation.withValue(actualValue);

  return violation;
}

ShaclViolation ViolationFactory::createNodeKindViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, NodeKind expectedKind,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:nodeKind");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' is not of expected node kind: "
      << nodeKindToString(expectedKind);
  violation.message = msg.str();

  violation.expectedValue = nodeKindToString(expectedKind);
  violation.withValue(actualValue);

  return violation;
}

ShaclViolation ViolationFactory::createPatternViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, const std::string& pattern,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:pattern");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' does not match pattern: " << pattern;
  violation.message = msg.str();

  violation.expectedValue = pattern;
  violation.withValue(actualValue);

  return violation;
}

ShaclViolation ViolationFactory::createMinLengthViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, int expectedMin,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:minLength");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' has length " << actualValue.length()
      << ", but minimum length is " << expectedMin;
  violation.message = msg.str();

  violation.expectedValue = std::to_string(expectedMin);
  violation.withValue(actualValue);
  violation.withDetail("actualLength", std::to_string(actualValue.length()));

  return violation;
}

ShaclViolation ViolationFactory::createMaxLengthViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, int expectedMax,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:maxLength");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' has length " << actualValue.length()
      << ", but maximum length is " << expectedMax;
  violation.message = msg.str();

  violation.expectedValue = std::to_string(expectedMax);
  violation.withValue(actualValue);
  violation.withDetail("actualLength", std::to_string(actualValue.length()));

  return violation;
}

ShaclViolation ViolationFactory::createInViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, const std::vector<std::string>& allowedValues,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:in");

  std::ostringstream msg;
  msg << "Value '" << actualValue << "' is not in allowed list: [";
  for (size_t i = 0; i < allowedValues.size(); ++i) {
    if (i > 0) msg << ", ";
    msg << allowedValues[i];
  }
  msg << "]";
  violation.message = msg.str();

  violation.withValue(actualValue);

  // Store allowed values in details
  std::ostringstream allowedStr;
  for (size_t i = 0; i < allowedValues.size(); ++i) {
    if (i > 0) allowedStr << ", ";
    allowedStr << allowedValues[i];
  }
  violation.withDetail("allowedValues", allowedStr.str());

  return violation;
}

ShaclViolation ViolationFactory::createMinInclusiveViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, const std::string& minValue,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:minInclusive");

  std::ostringstream msg;
  msg << "Value " << actualValue << " is less than minimum allowed value "
      << minValue;
  violation.message = msg.str();

  violation.expectedValue = minValue;
  violation.withValue(actualValue);

  return violation;
}

ShaclViolation ViolationFactory::createMaxInclusiveViolation(
    const std::string& focusNode, const std::string& path,
    const std::string& shapeId, const std::string& maxValue,
    const std::string& actualValue) {
  ShaclViolation violation(focusNode, path, shapeId, "sh:maxInclusive");

  std::ostringstream msg;
  msg << "Value " << actualValue << " is greater than maximum allowed value "
      << maxValue;
  violation.message = msg.str();

  violation.expectedValue = maxValue;
  violation.withValue(actualValue);

  return violation;
}

}  // namespace shacl
