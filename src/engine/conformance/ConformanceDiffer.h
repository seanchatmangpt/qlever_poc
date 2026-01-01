//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_DIFFER_H
#define QLEVER_CONFORMANCE_DIFFER_H

#include <cmath>
#include <string>
#include <vector>

#include "engine/conformance/ConformanceResultSchema.h"
#include "engine/conformance/ConformanceTestCase.h"
#include "util/json.h"

namespace ad_engine::conformance {

// Configuration for fuzzy matching
struct FuzzyMatchConfig {
  // Allow percentage tolerance for numeric comparisons (e.g., 0.1 for ±10%)
  double numericTolerance = 0.0;

  // Ignore array order when comparing
  bool ignoreArrayOrder = false;

  // Ignore specific fields when comparing
  std::vector<std::string> ignoredFields;
};

// Differ class for comparing actual vs expected output
class ConformanceDiffer {
 public:
  // Compare two JSON objects and produce a diff result
  static DiffResult compareJson(const nlohmann::json& actual,
                                const nlohmann::json& expected,
                                const FuzzyMatchConfig& config = {});

  // Deep JSON diff (recursive comparison)
  static void deepJsonDiff(const nlohmann::json& actual,
                           const nlohmann::json& expected,
                           std::vector<std::string>& differences,
                           const std::string& path,
                           const FuzzyMatchConfig& config);

  // Compare violations for validation tests
  // Checks counts, severity distribution, error types
  static DiffResult compareValidationResults(const nlohmann::json& actual,
                                             const nlohmann::json& expected);

  // Compare rule evaluation results
  // Checks fact counts, rule fire distributions, digests
  static DiffResult compareRuleResults(const nlohmann::json& actual,
                                       const nlohmann::json& expected);

  // Compare N3 verification results
  // Checks error codes, warnings
  static DiffResult compareN3Results(const nlohmann::json& actual,
                                     const nlohmann::json& expected);

  // Helper: Check if two numbers are approximately equal
  static bool approximatelyEqual(double a, double b, double tolerance);

  // Helper: Sort JSON array for order-independent comparison
  static nlohmann::json sortJsonArray(const nlohmann::json& arr);

  // Helper: Get value from nested JSON path (e.g., "violations.count")
  static std::optional<nlohmann::json> getJsonPath(const nlohmann::json& obj,
                                                   const std::string& path);
};

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_DIFFER_H
