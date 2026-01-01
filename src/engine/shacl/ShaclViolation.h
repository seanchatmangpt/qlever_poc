#ifndef QLEVER_ENGINE_SHACL_SHACLVIOLATION_H
#define QLEVER_ENGINE_SHACL_SHACLVIOLATION_H

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "ShaclShape.h"

namespace shacl {

// Detailed SHACL violation information
// Provides comprehensive context about what failed and why
class ShaclViolation {
 public:
  // The RDF resource (focus node) that failed validation
  std::string focusNode;

  // The property path where the violation occurred (e.g., "ex:name" or
  // "ex:friend/ex:name")
  std::string resultPath;

  // The shape that was violated (IRI of the NodeShape or PropertyShape)
  std::string sourceShape;

  // The specific constraint that was violated (e.g., "sh:minCount",
  // "sh:datatype")
  std::string sourceConstraintComponent;

  // The actual value(s) that caused the violation
  std::vector<std::string> values;

  // Expected value or constraint parameter (e.g., expected datatype,
  // min/max count)
  std::optional<std::string> expectedValue;

  // Severity level of the violation
  SeverityLevel severity = SeverityLevel::Violation;

  // Human-readable message describing the violation
  std::string message;

  // Additional context information (key-value pairs)
  std::unordered_map<std::string, std::string> details;

  // Constructors
  ShaclViolation() = default;

  ShaclViolation(std::string focus, std::string path, std::string shape,
                 std::string constraint, SeverityLevel sev = SeverityLevel::Violation)
      : focusNode(std::move(focus)),
        resultPath(std::move(path)),
        sourceShape(std::move(shape)),
        sourceConstraintComponent(std::move(constraint)),
        severity(sev) {}

  // Builder methods for fluent API
  ShaclViolation& withValue(const std::string& value) {
    values.push_back(value);
    return *this;
  }

  ShaclViolation& withValues(const std::vector<std::string>& vals) {
    values = vals;
    return *this;
  }

  ShaclViolation& withExpectedValue(const std::string& expected) {
    expectedValue = expected;
    return *this;
  }

  ShaclViolation& withMessage(const std::string& msg) {
    message = msg;
    return *this;
  }

  ShaclViolation& withDetail(const std::string& key,
                             const std::string& value) {
    details[key] = value;
    return *this;
  }

  // Utility methods
  bool isError() const { return severity == SeverityLevel::Violation; }
  bool isWarning() const { return severity == SeverityLevel::Warning; }
  bool isInfo() const { return severity == SeverityLevel::Info; }

  // Generate a default message if none provided
  std::string getOrGenerateMessage() const;

  // Get a short summary of the violation
  std::string getSummary() const;

  // Get detailed description with all context
  std::string getDetailedDescription() const;
};

// Group of violations organized by different criteria
class ViolationGroup {
 public:
  std::string groupKey;  // The key used for grouping (e.g., shape ID, constraint type)
  std::vector<ShaclViolation> violations;
  size_t violationCount = 0;
  size_t warningCount = 0;
  size_t infoCount = 0;

  void addViolation(const ShaclViolation& violation) {
    violations.push_back(violation);
    updateCounts(violation);
  }

  void addViolation(ShaclViolation&& violation) {
    updateCounts(violation);
    violations.push_back(std::move(violation));
  }

 private:
  void updateCounts(const ShaclViolation& v) {
    if (v.isError()) {
      violationCount++;
    } else if (v.isWarning()) {
      warningCount++;
    } else if (v.isInfo()) {
      infoCount++;
    }
  }
};

// Enhanced validation report with detailed violation tracking
class DetailedValidationReport {
 public:
  // Overall conformance
  bool conforms = true;

  // Total counts
  size_t totalViolations = 0;
  size_t totalWarnings = 0;
  size_t totalInfo = 0;

  // All violations (in order encountered)
  std::vector<ShaclViolation> violations;

  // Violations grouped by focus node
  std::unordered_map<std::string, std::vector<ShaclViolation>>
      violationsByFocusNode;

  // Violations grouped by shape
  std::unordered_map<std::string, std::vector<ShaclViolation>>
      violationsByShape;

  // Violations grouped by constraint type
  std::unordered_map<std::string, std::vector<ShaclViolation>>
      violationsByConstraint;

  // Violations grouped by severity
  std::unordered_map<SeverityLevel, std::vector<ShaclViolation>>
      violationsBySeverity;

  // Add a violation to the report
  void addViolation(const ShaclViolation& violation);
  void addViolation(ShaclViolation&& violation);

  // Query methods
  size_t getTotalIssues() const {
    return totalViolations + totalWarnings + totalInfo;
  }

  const std::vector<ShaclViolation>& getViolationsForNode(
      const std::string& focusNode) const;

  const std::vector<ShaclViolation>& getViolationsForShape(
      const std::string& shapeId) const;

  std::vector<ShaclViolation> getViolationsBySeverity(
      SeverityLevel severity) const;

  // Get summary statistics
  struct Summary {
    size_t totalResources = 0;
    size_t conformingResources = 0;
    size_t nonConformingResources = 0;
    size_t violations = 0;
    size_t warnings = 0;
    size_t infos = 0;
    std::unordered_map<std::string, size_t> violationsByShape;
    std::unordered_map<std::string, size_t> violationsByConstraint;
  };

  Summary getSummary() const;

  // Clear all violations
  void clear() {
    conforms = true;
    totalViolations = 0;
    totalWarnings = 0;
    totalInfo = 0;
    violations.clear();
    violationsByFocusNode.clear();
    violationsByShape.clear();
    violationsByConstraint.clear();
    violationsBySeverity.clear();
  }

 private:
  void updateIndices(const ShaclViolation& violation);
};

// Factory methods for creating common violations
class ViolationFactory {
 public:
  static ShaclViolation createMinCountViolation(const std::string& focusNode,
                                                 const std::string& path,
                                                 const std::string& shapeId,
                                                 int expectedMin,
                                                 int actualCount);

  static ShaclViolation createMaxCountViolation(const std::string& focusNode,
                                                 const std::string& path,
                                                 const std::string& shapeId,
                                                 int expectedMax,
                                                 int actualCount);

  static ShaclViolation createDatatypeViolation(const std::string& focusNode,
                                                 const std::string& path,
                                                 const std::string& shapeId,
                                                 const std::string& expectedType,
                                                 const std::string& actualValue);

  static ShaclViolation createNodeKindViolation(const std::string& focusNode,
                                                 const std::string& path,
                                                 const std::string& shapeId,
                                                 NodeKind expectedKind,
                                                 const std::string& actualValue);

  static ShaclViolation createPatternViolation(const std::string& focusNode,
                                                const std::string& path,
                                                const std::string& shapeId,
                                                const std::string& pattern,
                                                const std::string& actualValue);

  static ShaclViolation createMinLengthViolation(const std::string& focusNode,
                                                  const std::string& path,
                                                  const std::string& shapeId,
                                                  int expectedMin,
                                                  const std::string& actualValue);

  static ShaclViolation createMaxLengthViolation(const std::string& focusNode,
                                                  const std::string& path,
                                                  const std::string& shapeId,
                                                  int expectedMax,
                                                  const std::string& actualValue);

  static ShaclViolation createInViolation(
      const std::string& focusNode, const std::string& path,
      const std::string& shapeId,
      const std::vector<std::string>& allowedValues,
      const std::string& actualValue);

  static ShaclViolation createMinInclusiveViolation(
      const std::string& focusNode, const std::string& path,
      const std::string& shapeId, const std::string& minValue,
      const std::string& actualValue);

  static ShaclViolation createMaxInclusiveViolation(
      const std::string& focusNode, const std::string& path,
      const std::string& shapeId, const std::string& maxValue,
      const std::string& actualValue);

 private:
  static std::string nodeKindToString(NodeKind kind);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLVIOLATION_H
