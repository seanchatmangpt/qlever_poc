#include "ViolationFormatter.h"
#include <sstream>

namespace validation {

contracts::Violation ViolationFormatter::fromShaclViolation(
    const shacl::ShaclViolation& shaclViolation) {
  // Parse focus node
  contracts::RdfNode focusNode = parseRdfNode(shaclViolation.focusNode);

  // Create standardized violation
  contracts::Violation violation(
      focusNode,
      shaclViolation.sourceShape,
      convertSeverity(shaclViolation.severity));

  // Set result path
  if (!shaclViolation.resultPath.empty()) {
    violation.withResultPath(shaclViolation.resultPath);
  }

  // Set constraint component
  if (!shaclViolation.sourceConstraintComponent.empty()) {
    violation.withConstraintComponent(shaclViolation.sourceConstraintComponent);
  }

  // Set expected value
  if (shaclViolation.expectedValue.has_value()) {
    violation.withExpectedValue(*shaclViolation.expectedValue);
  }

  // Set message
  if (!shaclViolation.message.empty()) {
    violation.withMessage(shaclViolation.message);
  } else {
    // Generate default message if none provided
    violation.withMessage(shaclViolation.getOrGenerateMessage());
  }

  // Convert values
  for (const auto& valueStr : shaclViolation.values) {
    violation.withValue(parseRdfNode(valueStr));
  }

  // Preserve SHACL-specific metadata
  for (const auto& [key, value] : shaclViolation.details) {
    violation.withMetadata(key, value);
  }

  return violation;
}

std::vector<contracts::Violation> ViolationFormatter::fromShaclViolations(
    const std::vector<shacl::ShaclViolation>& shaclViolations) {
  std::vector<contracts::Violation> violations;
  violations.reserve(shaclViolations.size());

  for (const auto& shaclViolation : shaclViolations) {
    violations.push_back(fromShaclViolation(shaclViolation));
  }

  return violations;
}

std::vector<contracts::Violation> ViolationFormatter::fromShaclReport(
    const shacl::DetailedValidationReport& report) {
  return fromShaclViolations(report.violations);
}

nlohmann::json ViolationFormatter::toJsonArray(
    const std::vector<contracts::Violation>& violations) {
  nlohmann::json arr = nlohmann::json::array();

  for (const auto& violation : violations) {
    arr.push_back(violation.toJson());
  }

  return arr;
}

nlohmann::json ViolationFormatter::toJsonReport(
    const std::vector<contracts::Violation>& violations,
    bool conforms) {
  nlohmann::json report;
  report["conforms"] = conforms;
  report["violationCount"] = violations.size();
  report["violations"] = toJsonArray(violations);

  return report;
}

std::string ViolationFormatter::toTextReport(
    const std::vector<contracts::Violation>& violations,
    bool conforms) {
  std::ostringstream oss;

  oss << "Validation Report\n";
  oss << "=================\n";
  oss << "Conforms: " << (conforms ? "true" : "false") << "\n";
  oss << "Violations: " << violations.size() << "\n\n";

  if (!violations.empty()) {
    oss << "Violations:\n";
    oss << "-----------\n";

    size_t index = 1;
    for (const auto& violation : violations) {
      oss << index++ << ". " << violation.getSummary() << "\n";
      if (!violation.message.empty()) {
        oss << "   Message: " << violation.message << "\n";
      }
      oss << "\n";
    }
  }

  return oss.str();
}

contracts::RdfNode ViolationFormatter::parseRdfNode(
    const std::string& nodeStr) {
  if (nodeStr.empty()) {
    return contracts::RdfNode();
  }

  contracts::NodeType type = detectNodeType(nodeStr);

  // Handle IRI
  if (type == contracts::NodeType::IRI) {
    // Remove angle brackets
    if (nodeStr.size() >= 2 && nodeStr.front() == '<' &&
        nodeStr.back() == '>') {
      return contracts::RdfNode::iri(nodeStr.substr(1, nodeStr.size() - 2));
    }
    return contracts::RdfNode::iri(nodeStr);
  }

  // Handle blank node
  if (type == contracts::NodeType::BLANK_NODE) {
    // Remove _: prefix
    if (nodeStr.size() > 2 && nodeStr.substr(0, 2) == "_:") {
      return contracts::RdfNode::blankNode(nodeStr.substr(2));
    }
    return contracts::RdfNode::blankNode(nodeStr);
  }

  // Handle literal (simplified parsing)
  if (type == contracts::NodeType::LITERAL) {
    // Look for quotes
    size_t firstQuote = nodeStr.find('"');
    if (firstQuote == std::string::npos) {
      return contracts::RdfNode::literal(nodeStr);
    }

    size_t lastQuote = nodeStr.rfind('"');
    if (lastQuote == firstQuote) {
      return contracts::RdfNode::literal(nodeStr);
    }

    std::string value = nodeStr.substr(firstQuote + 1,
                                       lastQuote - firstQuote - 1);

    // Check for language tag (@lang)
    size_t atPos = nodeStr.find('@', lastQuote);
    if (atPos != std::string::npos) {
      std::string language = nodeStr.substr(atPos + 1);
      return contracts::RdfNode::literal(value, std::nullopt, language);
    }

    // Check for datatype (^^<type>)
    size_t caretPos = nodeStr.find("^^", lastQuote);
    if (caretPos != std::string::npos) {
      size_t dtStart = nodeStr.find('<', caretPos);
      size_t dtEnd = nodeStr.find('>', dtStart);
      if (dtStart != std::string::npos && dtEnd != std::string::npos) {
        std::string datatype = nodeStr.substr(dtStart + 1, dtEnd - dtStart - 1);
        return contracts::RdfNode::literal(value, datatype);
      }
    }

    return contracts::RdfNode::literal(value);
  }

  // Default: unknown
  return contracts::RdfNode(nodeStr, contracts::NodeType::UNKNOWN);
}

contracts::NodeType ViolationFormatter::detectNodeType(
    const std::string& nodeStr) {
  if (nodeStr.empty()) {
    return contracts::NodeType::UNKNOWN;
  }

  // Check for IRI (starts with < or contains ://)
  if (nodeStr.front() == '<' || nodeStr.find("://") != std::string::npos) {
    return contracts::NodeType::IRI;
  }

  // Check for blank node (starts with _:)
  if (nodeStr.size() > 2 && nodeStr.substr(0, 2) == "_:") {
    return contracts::NodeType::BLANK_NODE;
  }

  // Check for literal (contains quotes)
  if (nodeStr.find('"') != std::string::npos) {
    return contracts::NodeType::LITERAL;
  }

  // Default to IRI for plain URIs
  return contracts::NodeType::IRI;
}

contracts::ViolationSeverity ViolationFormatter::convertSeverity(
    shacl::SeverityLevel severity) {
  switch (severity) {
    case shacl::SeverityLevel::Violation:
      return contracts::ViolationSeverity::VIOLATION;
    case shacl::SeverityLevel::Warning:
      return contracts::ViolationSeverity::WARNING;
    case shacl::SeverityLevel::Info:
      return contracts::ViolationSeverity::INFO;
    default:
      return contracts::ViolationSeverity::VIOLATION;
  }
}

}  // namespace validation
