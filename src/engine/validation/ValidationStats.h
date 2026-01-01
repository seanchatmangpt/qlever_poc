#ifndef QLEVER_ENGINE_VALIDATION_VALIDATIONSTATS_H
#define QLEVER_ENGINE_VALIDATION_VALIDATIONSTATS_H

#include <cstddef>
#include <string>
#include <nlohmann/json.hpp>

namespace validation {

// Validation statistics and metrics
// Serializable to JSON for reporting
struct ValidationStats {
  // Violation counts
  size_t violationsFound = 0;
  size_t warningsFound = 0;
  size_t infosFound = 0;

  // Resource counts
  size_t focusNodesEvaluated = 0;
  size_t constraintsEvaluated = 0;

  // Performance metrics
  size_t runtimeMs = 0;

  // Guard triggers
  bool guardsTriggered = false;
  std::string guardTriggeredReason;

  // Cache statistics (optional)
  size_t cacheHits = 0;
  size_t cacheMisses = 0;

  // Total violations (convenience)
  size_t totalViolations() const {
    return violationsFound + warningsFound + infosFound;
  }

  // Cache hit rate
  double cacheHitRate() const {
    size_t total = cacheHits + cacheMisses;
    if (total == 0) return 0.0;
    return static_cast<double>(cacheHits) / static_cast<double>(total);
  }

  // JSON serialization
  nlohmann::json toJson() const {
    nlohmann::json j;
    j["violationsFound"] = violationsFound;
    j["warningsFound"] = warningsFound;
    j["infosFound"] = infosFound;
    j["totalViolations"] = totalViolations();
    j["focusNodesEvaluated"] = focusNodesEvaluated;
    j["constraintsEvaluated"] = constraintsEvaluated;
    j["runtimeMs"] = runtimeMs;
    j["guardsTriggered"] = guardsTriggered;

    if (guardsTriggered) {
      j["guardTriggeredReason"] = guardTriggeredReason;
    }

    if (cacheHits > 0 || cacheMisses > 0) {
      j["cacheHits"] = cacheHits;
      j["cacheMisses"] = cacheMisses;
      j["cacheHitRate"] = cacheHitRate();
    }

    return j;
  }

  // Summary string
  std::string getSummary() const {
    std::string result =
        "Violations: " + std::to_string(violationsFound) +
        ", Warnings: " + std::to_string(warningsFound) +
        ", Infos: " + std::to_string(infosFound) +
        ", Focus Nodes: " + std::to_string(focusNodesEvaluated) +
        ", Constraints: " + std::to_string(constraintsEvaluated) +
        ", Runtime: " + std::to_string(runtimeMs) + "ms";

    if (guardsTriggered) {
      result += ", Guards Triggered: " + guardTriggeredReason;
    }

    if (cacheHits > 0 || cacheMisses > 0) {
      result += ", Cache Hit Rate: " +
                std::to_string(static_cast<int>(cacheHitRate() * 100)) + "%";
    }

    return result;
  }
};

}  // namespace validation

#endif  // QLEVER_ENGINE_VALIDATION_VALIDATIONSTATS_H
