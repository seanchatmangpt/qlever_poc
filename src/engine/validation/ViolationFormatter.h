#ifndef QLEVER_ENGINE_VALIDATION_VIOLATIONFORMATTER_H
#define QLEVER_ENGINE_VALIDATION_VIOLATIONFORMATTER_H

#include "engine/contracts/Violation.h"
#include "engine/contracts/CommonTypes.h"
#include "engine/shacl/ShaclViolation.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace validation {

// Formatter for converting validator-specific violations to standardized format
// Shared formatting logic for SHACL and ShEx violations
class ViolationFormatter {
 public:
  // Convert SHACL violation to standardized violation
  static contracts::Violation fromShaclViolation(
      const shacl::ShaclViolation& shaclViolation);

  // Convert multiple SHACL violations
  static std::vector<contracts::Violation> fromShaclViolations(
      const std::vector<shacl::ShaclViolation>& shaclViolations);

  // Convert SHACL validation report to standardized violations
  static std::vector<contracts::Violation> fromShaclReport(
      const shacl::DetailedValidationReport& report);

  // Format violations as JSON array
  static nlohmann::json toJsonArray(
      const std::vector<contracts::Violation>& violations);

  // Format violations as JSON report
  static nlohmann::json toJsonReport(
      const std::vector<contracts::Violation>& violations,
      bool conforms);

  // Format violations as human-readable text
  static std::string toTextReport(
      const std::vector<contracts::Violation>& violations,
      bool conforms);

  // Helper: Parse RDF node from string representation
  static contracts::RdfNode parseRdfNode(const std::string& nodeStr);

  // Helper: Detect node type from string representation
  static contracts::NodeType detectNodeType(const std::string& nodeStr);

  // Helper: Convert SHACL severity to standardized severity
  static contracts::ViolationSeverity convertSeverity(
      shacl::SeverityLevel severity);
};

}  // namespace validation

#endif  // QLEVER_ENGINE_VALIDATION_VIOLATIONFORMATTER_H
