//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#include "engine/conformance/ConformanceDiffer.h"

#include <algorithm>
#include <sstream>

namespace ad_engine::conformance {

DiffResult ConformanceDiffer::compareJson(const nlohmann::json& actual,
                                          const nlohmann::json& expected,
                                          const FuzzyMatchConfig& config) {
  DiffResult result;
  result.actual = actual;
  result.expected = expected;

  // Perform deep comparison
  deepJsonDiff(actual, expected, result.differences, "", config);

  result.hasDifference = !result.differences.empty();
  return result;
}

void ConformanceDiffer::deepJsonDiff(const nlohmann::json& actual,
                                     const nlohmann::json& expected,
                                     std::vector<std::string>& differences,
                                     const std::string& path,
                                     const FuzzyMatchConfig& config) {
  // Check if field should be ignored
  if (std::find(config.ignoredFields.begin(), config.ignoredFields.end(),
                path) != config.ignoredFields.end()) {
    return;
  }

  // Type mismatch
  if (actual.type() != expected.type()) {
    std::stringstream ss;
    ss << "Type mismatch at " << (path.empty() ? "root" : path) << ": actual="
       << jsonToTypeString(actual) << ", expected=" << jsonToTypeString(expected);
    differences.push_back(ss.str());
    return;
  }

  // Compare based on type
  if (actual.is_null()) {
    // Both null, no difference
    return;
  } else if (actual.is_boolean()) {
    if (actual.get<bool>() != expected.get<bool>()) {
      std::stringstream ss;
      ss << "Boolean mismatch at " << (path.empty() ? "root" : path)
         << ": actual=" << actual << ", expected=" << expected;
      differences.push_back(ss.str());
    }
  } else if (actual.is_number()) {
    double actualNum = actual.get<double>();
    double expectedNum = expected.get<double>();
    if (!approximatelyEqual(actualNum, expectedNum, config.numericTolerance)) {
      std::stringstream ss;
      ss << "Number mismatch at " << (path.empty() ? "root" : path)
         << ": actual=" << actualNum << ", expected=" << expectedNum;
      differences.push_back(ss.str());
    }
  } else if (actual.is_string()) {
    if (actual.get<std::string>() != expected.get<std::string>()) {
      std::stringstream ss;
      ss << "String mismatch at " << (path.empty() ? "root" : path)
         << ": actual=\"" << actual.get<std::string>() << "\", expected=\""
         << expected.get<std::string>() << "\"";
      differences.push_back(ss.str());
    }
  } else if (actual.is_array()) {
    nlohmann::json actualArray = actual;
    nlohmann::json expectedArray = expected;

    // If ignoring array order, sort both arrays
    if (config.ignoreArrayOrder) {
      actualArray = sortJsonArray(actual);
      expectedArray = sortJsonArray(expected);
    }

    if (actualArray.size() != expectedArray.size()) {
      std::stringstream ss;
      ss << "Array size mismatch at " << (path.empty() ? "root" : path)
         << ": actual=" << actualArray.size()
         << ", expected=" << expectedArray.size();
      differences.push_back(ss.str());
      return;
    }

    for (size_t i = 0; i < actualArray.size(); ++i) {
      std::string elementPath =
          path + "[" + std::to_string(i) + "]";
      deepJsonDiff(actualArray[i], expectedArray[i], differences, elementPath,
                   config);
    }
  } else if (actual.is_object()) {
    // Get all keys from both objects
    std::vector<std::string> actualKeys;
    std::vector<std::string> expectedKeys;

    for (auto it = actual.begin(); it != actual.end(); ++it) {
      actualKeys.push_back(it.key());
    }
    for (auto it = expected.begin(); it != expected.end(); ++it) {
      expectedKeys.push_back(it.key());
    }

    // Sort keys for deterministic comparison
    std::sort(actualKeys.begin(), actualKeys.end());
    std::sort(expectedKeys.begin(), expectedKeys.end());

    // Check for missing keys in actual
    for (const auto& key : expectedKeys) {
      if (actual.find(key) == actual.end()) {
        std::stringstream ss;
        ss << "Missing key in actual at " << (path.empty() ? "root" : path)
           << ": \"" << key << "\"";
        differences.push_back(ss.str());
      }
    }

    // Check for extra keys in actual
    for (const auto& key : actualKeys) {
      if (expected.find(key) == expected.end()) {
        std::stringstream ss;
        ss << "Extra key in actual at " << (path.empty() ? "root" : path)
           << ": \"" << key << "\"";
        differences.push_back(ss.str());
      }
    }

    // Compare common keys
    for (const auto& key : expectedKeys) {
      if (actual.find(key) != actual.end()) {
        std::string fieldPath = path.empty() ? key : path + "." + key;
        deepJsonDiff(actual[key], expected[key], differences, fieldPath,
                     config);
      }
    }
  }
}

DiffResult ConformanceDiffer::compareValidationResults(
    const nlohmann::json& actual, const nlohmann::json& expected) {
  DiffResult result;
  result.actual = actual;
  result.expected = expected;

  // Compare violation counts
  if (actual.contains("violations") && expected.contains("violations")) {
    const auto& actualViolations = actual["violations"];
    const auto& expectedViolations = expected["violations"];

    if (actualViolations.is_array() && expectedViolations.is_array()) {
      size_t actualCount = actualViolations.size();
      size_t expectedCount = expectedViolations.size();

      if (actualCount != expectedCount) {
        std::stringstream ss;
        ss << "Violation count mismatch: actual=" << actualCount
           << ", expected=" << expectedCount;
        result.differences.push_back(ss.str());
      }
    }
  }

  // Compare severity distribution
  if (actual.contains("severity_distribution") &&
      expected.contains("severity_distribution")) {
    FuzzyMatchConfig config;
    config.ignoreArrayOrder = true;
    deepJsonDiff(actual["severity_distribution"],
                 expected["severity_distribution"], result.differences,
                 "severity_distribution", config);
  }

  result.hasDifference = !result.differences.empty();
  return result;
}

DiffResult ConformanceDiffer::compareRuleResults(const nlohmann::json& actual,
                                                 const nlohmann::json& expected) {
  DiffResult result;
  result.actual = actual;
  result.expected = expected;

  // Compare fact counts
  if (actual.contains("fact_count") && expected.contains("fact_count")) {
    size_t actualCount = actual["fact_count"].get<size_t>();
    size_t expectedCount = expected["fact_count"].get<size_t>();

    if (actualCount != expectedCount) {
      std::stringstream ss;
      ss << "Fact count mismatch: actual=" << actualCount
         << ", expected=" << expectedCount;
      result.differences.push_back(ss.str());
    }
  }

  // Compare rule fire distributions
  if (actual.contains("rule_fires") && expected.contains("rule_fires")) {
    FuzzyMatchConfig config;
    config.ignoreArrayOrder = true;
    deepJsonDiff(actual["rule_fires"], expected["rule_fires"],
                 result.differences, "rule_fires", config);
  }

  // Compare digests for reproducibility
  if (actual.contains("digest") && expected.contains("digest")) {
    std::string actualDigest = actual["digest"].get<std::string>();
    std::string expectedDigest = expected["digest"].get<std::string>();

    if (actualDigest != expectedDigest) {
      std::stringstream ss;
      ss << "Digest mismatch: actual=" << actualDigest
         << ", expected=" << expectedDigest;
      result.differences.push_back(ss.str());
    }
  }

  result.hasDifference = !result.differences.empty();
  return result;
}

DiffResult ConformanceDiffer::compareN3Results(const nlohmann::json& actual,
                                               const nlohmann::json& expected) {
  DiffResult result;
  result.actual = actual;
  result.expected = expected;

  // Compare error codes
  if (actual.contains("error_code") && expected.contains("error_code")) {
    std::string actualError = actual["error_code"].get<std::string>();
    std::string expectedError = expected["error_code"].get<std::string>();

    if (actualError != expectedError) {
      std::stringstream ss;
      ss << "Error code mismatch: actual=" << actualError
         << ", expected=" << expectedError;
      result.differences.push_back(ss.str());
    }
  }

  // Compare warnings
  if (actual.contains("warnings") && expected.contains("warnings")) {
    FuzzyMatchConfig config;
    config.ignoreArrayOrder = true;
    deepJsonDiff(actual["warnings"], expected["warnings"], result.differences,
                 "warnings", config);
  }

  result.hasDifference = !result.differences.empty();
  return result;
}

bool ConformanceDiffer::approximatelyEqual(double a, double b,
                                           double tolerance) {
  if (tolerance == 0.0) {
    return a == b;
  }

  double diff = std::abs(a - b);
  double avg = (std::abs(a) + std::abs(b)) / 2.0;

  // Relative tolerance comparison
  return diff <= avg * tolerance;
}

nlohmann::json ConformanceDiffer::sortJsonArray(const nlohmann::json& arr) {
  if (!arr.is_array()) {
    return arr;
  }

  std::vector<nlohmann::json> elements;
  for (const auto& item : arr) {
    elements.push_back(item);
  }

  // Sort by JSON string representation (simple but deterministic)
  std::sort(elements.begin(), elements.end(),
            [](const nlohmann::json& a, const nlohmann::json& b) {
              return a.dump() < b.dump();
            });

  nlohmann::json result = nlohmann::json::array();
  for (const auto& item : elements) {
    result.push_back(item);
  }

  return result;
}

std::optional<nlohmann::json> ConformanceDiffer::getJsonPath(
    const nlohmann::json& obj, const std::string& path) {
  // Simple path implementation (supports "key1.key2.key3" notation)
  std::vector<std::string> pathParts;
  std::stringstream ss(path);
  std::string part;

  while (std::getline(ss, part, '.')) {
    pathParts.push_back(part);
  }

  nlohmann::json current = obj;
  for (const auto& key : pathParts) {
    if (!current.is_object() || !current.contains(key)) {
      return std::nullopt;
    }
    current = current[key];
  }

  return current;
}

}  // namespace ad_engine::conformance
