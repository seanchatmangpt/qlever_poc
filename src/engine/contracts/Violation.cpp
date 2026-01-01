// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#include "engine/contracts/Violation.h"

#include <algorithm>

namespace qlever::contracts {

nlohmann::json Violation::toJson() const {
  nlohmann::json j;
  j["violationType"] = static_cast<int>(violationType);
  j["violationTypeString"] = std::string(getViolationTypeString());
  j["severity"] = static_cast<int>(severity);
  j["severityString"] = std::string(getSeverityString());
  j["message"] = message;

  // Convert affected nodes to array of ID bits
  nlohmann::json nodesArray = nlohmann::json::array();
  for (const auto& id : affectedNodes) {
    nodesArray.push_back(id.getBits());
  }
  j["affectedNodes"] = nodesArray;

  if (shapeReference.has_value()) {
    j["shapeReference"] = shapeReference.value();
  }

  j["timestampMs"] = timestampMs;

  if (sourceLocation.has_value()) {
    j["sourceLocation"] = sourceLocation.value();
  }

  if (contextData.has_value()) {
    j["contextData"] = contextData.value();
  }

  return j;
}

Violation Violation::fromJson(const nlohmann::json& j) {
  Violation v;

  v.violationType = static_cast<ErrorCode>(j["violationType"].get<int>());
  v.severity = static_cast<Severity>(j["severity"].get<int>());
  v.message = j["message"].get<std::string>();

  // Convert nodes array back to Id vector
  if (j.contains("affectedNodes") && j["affectedNodes"].is_array()) {
    for (const auto& nodeJson : j["affectedNodes"]) {
      v.affectedNodes.push_back(Id::makeFromInt(nodeJson.get<uint64_t>()));
    }
  }

  if (j.contains("shapeReference") && !j["shapeReference"].is_null()) {
    v.shapeReference = j["shapeReference"].get<std::string>();
  }

  v.timestampMs = j["timestampMs"].get<uint64_t>();

  if (j.contains("sourceLocation") && !j["sourceLocation"].is_null()) {
    v.sourceLocation = j["sourceLocation"].get<std::string>();
  }

  if (j.contains("contextData") && !j["contextData"].is_null()) {
    v.contextData = j["contextData"];
  }

  return v;
}

size_t ViolationReport::countBySeverity(Severity sev) const {
  return std::count_if(violations_.begin(), violations_.end(),
                       [sev](const Violation& v) { return v.severity == sev; });
}

size_t ViolationReport::countByType(ErrorCode type) const {
  return std::count_if(violations_.begin(), violations_.end(),
                       [type](const Violation& v) { return v.violationType == type; });
}

bool ViolationReport::hasErrors() const {
  return std::any_of(violations_.begin(), violations_.end(),
                     [](const Violation& v) { return v.isError(); });
}

std::optional<Severity> ViolationReport::getMaxSeverity() const {
  if (violations_.empty()) {
    return std::nullopt;
  }

  auto maxIt = std::max_element(
      violations_.begin(), violations_.end(),
      [](const Violation& a, const Violation& b) {
        return static_cast<int>(a.severity) < static_cast<int>(b.severity);
      });

  return maxIt->severity;
}

nlohmann::json ViolationReport::toJson() const {
  nlohmann::json j;
  j["count"] = violations_.size();
  j["hasErrors"] = hasErrors();

  auto maxSev = getMaxSeverity();
  if (maxSev.has_value()) {
    j["maxSeverity"] = static_cast<int>(maxSev.value());
    j["maxSeverityString"] = std::string(severityToString(maxSev.value()));
  }

  // Violations array
  nlohmann::json violationsArray = nlohmann::json::array();
  for (const auto& violation : violations_) {
    violationsArray.push_back(violation.toJson());
  }
  j["violations"] = violationsArray;

  // Severity counts
  nlohmann::json severityCounts;
  severityCounts["INFO"] = countBySeverity(Severity::INFO);
  severityCounts["WARNING"] = countBySeverity(Severity::WARNING);
  severityCounts["ERROR"] = countBySeverity(Severity::ERROR);
  severityCounts["CRITICAL"] = countBySeverity(Severity::CRITICAL);
  j["severityCounts"] = severityCounts;

  return j;
}

ViolationReport ViolationReport::fromJson(const nlohmann::json& j) {
  ViolationReport report;

  if (j.contains("violations") && j["violations"].is_array()) {
    for (const auto& vJson : j["violations"]) {
      report.add(Violation::fromJson(vJson));
    }
  }

  return report;
}

}  // namespace qlever::contracts
