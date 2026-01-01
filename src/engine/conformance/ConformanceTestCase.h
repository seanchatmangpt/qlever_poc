//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_TEST_CASE_H
#define QLEVER_CONFORMANCE_TEST_CASE_H

#include <string>
#include <unordered_map>
#include <variant>

#include "util/json.h"

namespace ad_engine::conformance {

// Supported test types for conformance testing
enum class TestType {
  Validation,    // SHACL/ShEx validation tests
  Rules,         // Datalog rule evaluation tests
  N3Verify,      // N3 reasoning verification tests
  Performance    // Performance benchmarking tests
};

// Expected outcome types
enum class ExpectedOutcome {
  Pass,           // Test should pass without violations
  Violations,     // Test should produce violations
  Error,          // Test should produce an error
  Timeout,        // Test should timeout
  Skip            // Test should be skipped
};

// Convert TestType to string for JSON serialization
inline std::string testTypeToString(TestType type) {
  switch (type) {
    case TestType::Validation:
      return "Validation";
    case TestType::Rules:
      return "Rules";
    case TestType::N3Verify:
      return "N3Verify";
    case TestType::Performance:
      return "Performance";
    default:
      return "Unknown";
  }
}

// Convert ExpectedOutcome to string for JSON serialization
inline std::string expectedOutcomeToString(ExpectedOutcome outcome) {
  switch (outcome) {
    case ExpectedOutcome::Pass:
      return "PASS";
    case ExpectedOutcome::Violations:
      return "VIOLATIONS";
    case ExpectedOutcome::Error:
      return "ERROR";
    case ExpectedOutcome::Timeout:
      return "TIMEOUT";
    case ExpectedOutcome::Skip:
      return "SKIP";
    default:
      return "UNKNOWN";
  }
}

// Parse string to TestType
inline TestType stringToTestType(const std::string& str) {
  if (str == "Validation") return TestType::Validation;
  if (str == "Rules") return TestType::Rules;
  if (str == "N3Verify") return TestType::N3Verify;
  if (str == "Performance") return TestType::Performance;
  throw std::runtime_error("Unknown test type: " + str);
}

// Parse string to ExpectedOutcome
inline ExpectedOutcome stringToExpectedOutcome(const std::string& str) {
  if (str == "PASS") return ExpectedOutcome::Pass;
  if (str == "VIOLATIONS") return ExpectedOutcome::Violations;
  if (str == "ERROR") return ExpectedOutcome::Error;
  if (str == "TIMEOUT") return ExpectedOutcome::Timeout;
  if (str == "SKIP") return ExpectedOutcome::Skip;
  throw std::runtime_error("Unknown expected outcome: " + str);
}

// Generic test case abstraction supporting polymorphic test types
struct ConformanceTestCase {
  // Test identification
  std::string testName;
  TestType testType;

  // File paths (all paths are parameterized, never hardcoded)
  std::string inputPath;
  std::string expectedOutputPath;

  // Expected outcome
  ExpectedOutcome expectedOutcome;

  // Optional metadata for extensibility
  std::unordered_map<std::string, std::string> metadata;

  // Optional timeout in milliseconds (0 means no timeout)
  size_t timeoutMs = 0;

  // Optional tags for filtering tests
  std::vector<std::string> tags;

  // Default constructor
  ConformanceTestCase() = default;

  // Convenience constructor
  ConformanceTestCase(std::string name, TestType type, std::string input,
                      std::string expectedOutput,
                      ExpectedOutcome outcome = ExpectedOutcome::Pass)
      : testName(std::move(name)),
        testType(type),
        inputPath(std::move(input)),
        expectedOutputPath(std::move(expectedOutput)),
        expectedOutcome(outcome) {}

  // JSON serialization support
  nlohmann::json toJson() const {
    nlohmann::json j;
    j["test_name"] = testName;
    j["test_type"] = testTypeToString(testType);
    j["input_path"] = inputPath;
    j["expected_output_path"] = expectedOutputPath;
    j["expected_outcome"] = expectedOutcomeToString(expectedOutcome);
    if (timeoutMs > 0) {
      j["timeout_ms"] = timeoutMs;
    }
    if (!tags.empty()) {
      j["tags"] = tags;
    }
    if (!metadata.empty()) {
      j["metadata"] = metadata;
    }
    return j;
  }

  // JSON deserialization support
  static ConformanceTestCase fromJson(const nlohmann::json& j) {
    ConformanceTestCase testCase;
    testCase.testName = j.at("test_name").get<std::string>();
    testCase.testType = stringToTestType(j.at("test_type").get<std::string>());
    testCase.inputPath = j.at("input_path").get<std::string>();
    testCase.expectedOutputPath =
        j.at("expected_output_path").get<std::string>();
    testCase.expectedOutcome =
        stringToExpectedOutcome(j.at("expected_outcome").get<std::string>());

    if (j.contains("timeout_ms")) {
      testCase.timeoutMs = j.at("timeout_ms").get<size_t>();
    }
    if (j.contains("tags")) {
      testCase.tags = j.at("tags").get<std::vector<std::string>>();
    }
    if (j.contains("metadata")) {
      testCase.metadata =
          j.at("metadata")
              .get<std::unordered_map<std::string, std::string>>();
    }

    return testCase;
  }
};

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_TEST_CASE_H
