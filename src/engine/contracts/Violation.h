// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_CONTRACTS_VIOLATION_H
#define QLEVER_SRC_ENGINE_CONTRACTS_VIOLATION_H

#include <optional>
#include <string>
#include <vector>

#include "engine/contracts/CommonTypes.h"
#include "engine/contracts/ErrorCode.h"
#include "global/Id.h"
#include "util/json.h"

namespace qlever::contracts {

// Generic violation representation for constraint violations
// Works for both SHACL and ShEx violations, as well as rules/N3 errors
struct Violation {
  // Type of violation (from ErrorCode taxonomy)
  ErrorCode violationType;

  // Severity level of this violation
  Severity severity;

  // Human-readable message describing the violation
  std::string message;

  // Affected node IDs (e.g., focus node, value nodes)
  std::vector<Id> affectedNodes;

  // Optional reference to the shape/rule that was violated
  std::optional<std::string> shapeReference;

  // Timestamp when violation was detected (milliseconds since epoch)
  uint64_t timestampMs;

  // Optional source location (file, line number) for debugging
  std::optional<std::string> sourceLocation;

  // Optional additional context data
  std::optional<nlohmann::json> contextData;

  // Default constructor
  Violation() = default;

  // Convenience constructor for simple violations
  Violation(ErrorCode type, Severity sev, std::string msg)
      : violationType(type),
        severity(sev),
        message(std::move(msg)),
        timestampMs(currentTimestampMs()) {}

  // Full constructor
  Violation(ErrorCode type, Severity sev, std::string msg,
            std::vector<Id> nodes, std::optional<std::string> shapeRef = std::nullopt)
      : violationType(type),
        severity(sev),
        message(std::move(msg)),
        affectedNodes(std::move(nodes)),
        shapeReference(std::move(shapeRef)),
        timestampMs(currentTimestampMs()) {}

  // Convert to JSON for serialization
  nlohmann::json toJson() const;

  // Create from JSON for deserialization
  static Violation fromJson(const nlohmann::json& j);

  // Get violation type as string
  std::string_view getViolationTypeString() const {
    return errorCodeDescription(violationType);
  }

  // Get severity as string
  std::string_view getSeverityString() const {
    return severityToString(severity);
  }

  // Check if this is a critical violation
  bool isCritical() const { return severity == Severity::CRITICAL; }

  // Check if this is an error-level violation
  bool isError() const {
    return severity == Severity::ERROR || severity == Severity::CRITICAL;
  }
};

// Collection of violations with aggregation helpers
class ViolationReport {
 private:
  std::vector<Violation> violations_;

 public:
  ViolationReport() = default;

  // Add a single violation
  void add(Violation v) { violations_.push_back(std::move(v)); }

  // Add multiple violations
  void addAll(const std::vector<Violation>& vs) {
    violations_.insert(violations_.end(), vs.begin(), vs.end());
  }

  // Get all violations
  const std::vector<Violation>& getViolations() const { return violations_; }

  // Get number of violations
  size_t count() const { return violations_.size(); }

  // Check if empty
  bool empty() const { return violations_.empty(); }

  // Clear all violations
  void clear() { violations_.clear(); }

  // Count violations by severity
  size_t countBySeverity(Severity sev) const;

  // Count violations by type
  size_t countByType(ErrorCode type) const;

  // Check if report contains any errors or critical violations
  bool hasErrors() const;

  // Get the highest severity level in the report
  std::optional<Severity> getMaxSeverity() const;

  // Convert to JSON
  nlohmann::json toJson() const;

  // Create from JSON
  static ViolationReport fromJson(const nlohmann::json& j);
};

}  // namespace qlever::contracts

#endif  // QLEVER_SRC_ENGINE_CONTRACTS_VIOLATION_H
