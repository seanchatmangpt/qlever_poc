//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_RESULT_SCHEMA_H
#define QLEVER_CONFORMANCE_RESULT_SCHEMA_H

#include <chrono>
#include <string>
#include <vector>

#include "engine/conformance/ConformanceTestCase.h"
#include "util/json.h"

namespace ad_engine::conformance {

// Status of a single test case result
enum class TestStatus {
  Pass,    // Test passed (actual == expected)
  Fail,    // Test failed (actual != expected)
  Skip,    // Test was skipped
  Error,   // Test encountered an error
  Timeout  // Test timed out
};

// Convert TestStatus to string
inline std::string testStatusToString(TestStatus status) {
  switch (status) {
    case TestStatus::Pass:
      return "PASS";
    case TestStatus::Fail:
      return "FAIL";
    case TestStatus::Skip:
      return "SKIP";
    case TestStatus::Error:
      return "ERROR";
    case TestStatus::Timeout:
      return "TIMEOUT";
    default:
      return "UNKNOWN";
  }
}

// Parse string to TestStatus
inline TestStatus stringToTestStatus(const std::string& str) {
  if (str == "PASS") return TestStatus::Pass;
  if (str == "FAIL") return TestStatus::Fail;
  if (str == "SKIP") return TestStatus::Skip;
  if (str == "ERROR") return TestStatus::Error;
  if (str == "TIMEOUT") return TestStatus::Timeout;
  throw std::runtime_error("Unknown test status: " + str);
}

// Diff result structure for comparing actual vs expected
struct DiffResult {
  bool hasDifference = false;
  std::vector<std::string> differences;
  nlohmann::json actual;
  nlohmann::json expected;

  // Serialize to JSON
  nlohmann::json toJson() const {
    nlohmann::json j;
    j["has_difference"] = hasDifference;
    if (hasDifference) {
      j["differences"] = differences;
      j["actual"] = actual;
      j["expected"] = expected;
    }
    return j;
  }
};

// Result of a single test case execution
struct TestCaseResult {
  std::string testName;
  TestStatus status;
  ExpectedOutcome expectedOutcome;
  std::string actualOutcome;

  // Timing information
  double timingMs = 0.0;

  // Detailed results (varies by test type)
  nlohmann::json actualData;
  nlohmann::json expectedData;

  // Diff information
  DiffResult diff;

  // Error message (if status is Error)
  std::string errorMessage;

  // Additional metrics (for validation tests)
  std::optional<size_t> violationsExpected;
  std::optional<size_t> violationsActual;

  // Additional metrics (for rule tests)
  std::optional<size_t> factsExpected;
  std::optional<size_t> factsActual;
  std::optional<std::string> digestExpected;
  std::optional<std::string> digestActual;

  // Check if test passed
  bool passed() const { return status == TestStatus::Pass; }

  // Serialize to JSON with stable ordering
  nlohmann::json toJson() const {
    nlohmann::ordered_json j;
    j["test_name"] = testName;
    j["status"] = testStatusToString(status);
    j["expected_outcome"] = expectedOutcomeToString(expectedOutcome);
    j["actual_outcome"] = actualOutcome;
    j["timing_ms"] = timingMs;
    j["passed"] = passed();

    if (violationsExpected.has_value()) {
      j["violations_expected"] = violationsExpected.value();
    }
    if (violationsActual.has_value()) {
      j["violations_actual"] = violationsActual.value();
    }
    if (factsExpected.has_value()) {
      j["facts_expected"] = factsExpected.value();
    }
    if (factsActual.has_value()) {
      j["facts_actual"] = factsActual.value();
    }
    if (digestExpected.has_value()) {
      j["digest_expected"] = digestExpected.value();
    }
    if (digestActual.has_value()) {
      j["digest_actual"] = digestActual.value();
    }

    if (!errorMessage.empty()) {
      j["error_message"] = errorMessage;
    }

    if (diff.hasDifference) {
      j["diff"] = diff.toJson();
    }

    return j;
  }

  // Deserialize from JSON
  static TestCaseResult fromJson(const nlohmann::json& j) {
    TestCaseResult result;
    result.testName = j.at("test_name").get<std::string>();
    result.status = stringToTestStatus(j.at("status").get<std::string>());
    result.expectedOutcome =
        stringToExpectedOutcome(j.at("expected_outcome").get<std::string>());
    result.actualOutcome = j.at("actual_outcome").get<std::string>();
    result.timingMs = j.at("timing_ms").get<double>();

    if (j.contains("violations_expected")) {
      result.violationsExpected = j.at("violations_expected").get<size_t>();
    }
    if (j.contains("violations_actual")) {
      result.violationsActual = j.at("violations_actual").get<size_t>();
    }
    if (j.contains("facts_expected")) {
      result.factsExpected = j.at("facts_expected").get<size_t>();
    }
    if (j.contains("facts_actual")) {
      result.factsActual = j.at("facts_actual").get<size_t>();
    }
    if (j.contains("digest_expected")) {
      result.digestExpected = j.at("digest_expected").get<std::string>();
    }
    if (j.contains("digest_actual")) {
      result.digestActual = j.at("digest_actual").get<std::string>();
    }
    if (j.contains("error_message")) {
      result.errorMessage = j.at("error_message").get<std::string>();
    }

    return result;
  }
};

// Summary statistics for a test run
struct TestRunSummary {
  size_t total = 0;
  size_t passed = 0;
  size_t failed = 0;
  size_t skipped = 0;
  size_t errors = 0;
  size_t timeouts = 0;
  double totalTimeMs = 0.0;

  // Calculate derived statistics
  double passRate() const {
    return total > 0 ? (static_cast<double>(passed) / total) * 100.0 : 0.0;
  }

  // Serialize to JSON with stable ordering
  nlohmann::json toJson() const {
    nlohmann::ordered_json j;
    j["total"] = total;
    j["passed"] = passed;
    j["failed"] = failed;
    j["skipped"] = skipped;
    j["errors"] = errors;
    j["timeouts"] = timeouts;
    j["pass_rate_percent"] = passRate();
    j["total_time_ms"] = totalTimeMs;
    return j;
  }
};

// Complete runner output structure
struct ConformanceRunnerResult {
  std::string runnerName;
  std::string timestampIso8601;
  std::vector<TestCaseResult> testResults;
  TestRunSummary summary;

  // Metadata about the test run
  std::unordered_map<std::string, std::string> runMetadata;

  // Serialize to JSON with stable ordering
  nlohmann::json toJson() const {
    nlohmann::ordered_json j;
    j["runner_name"] = runnerName;
    j["timestamp_iso8601"] = timestampIso8601;

    // Serialize test results
    nlohmann::ordered_json testsArray = nlohmann::ordered_json::array();
    for (const auto& result : testResults) {
      testsArray.push_back(result.toJson());
    }
    j["tests"] = testsArray;

    // Serialize summary
    j["summary"] = summary.toJson();

    // Add metadata if present
    if (!runMetadata.empty()) {
      j["metadata"] = runMetadata;
    }

    return j;
  }

  // Deserialize from JSON
  static ConformanceRunnerResult fromJson(const nlohmann::json& j) {
    ConformanceRunnerResult result;
    result.runnerName = j.at("runner_name").get<std::string>();
    result.timestampIso8601 = j.at("timestamp_iso8601").get<std::string>();

    // Deserialize test results
    if (j.contains("tests")) {
      for (const auto& testJson : j.at("tests")) {
        result.testResults.push_back(TestCaseResult::fromJson(testJson));
      }
    }

    if (j.contains("metadata")) {
      result.runMetadata =
          j.at("metadata")
              .get<std::unordered_map<std::string, std::string>>();
    }

    return result;
  }
};

// Helper function to get current ISO8601 timestamp
inline std::string getCurrentTimestampIso8601() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::gmtime(&time_t);

  char buffer[100];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buffer);
}

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_RESULT_SCHEMA_H
