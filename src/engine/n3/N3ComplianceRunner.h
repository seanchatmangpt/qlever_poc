// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Conformance Test Runner
// Loads N3 test cases and runs verification, producing JSON output

#ifndef QLEVER_SRC_ENGINE_N3_N3COMPLIANCERUNNER_H
#define QLEVER_SRC_ENGINE_N3_N3COMPLIANCERUNNER_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "engine/n3/N3Config.h"
#include "engine/n3/N3Service.h"
#include "engine/n3/N3VerifyResult.h"

namespace ad_engine::n3 {

// Test case for N3 conformance testing
struct N3TestCase {
  std::string name;
  std::string input_file;
  bool should_pass;  // Expected result (true = valid, false = invalid)
  std::string description;

  nlohmann::json toJson() const {
    return nlohmann::json{{"name", name},
                          {"input_file", input_file},
                          {"should_pass", should_pass},
                          {"description", description}};
  }

  static N3TestCase fromJson(const nlohmann::json& j) {
    N3TestCase test;
    test.name = j.value("name", std::string{});
    test.input_file = j.value("input_file", std::string{});
    test.should_pass = j.value("should_pass", true);
    test.description = j.value("description", std::string{});
    return test;
  }
};

// Result of running a single test case
struct N3TestResult {
  std::string test_name;
  bool passed;  // Test assertion passed (result matches expectation)
  N3VerifyResult verify_result;
  std::string error_message;  // If test assertion failed

  nlohmann::json toJson() const {
    nlohmann::json j;
    j["test_name"] = test_name;
    j["passed"] = passed;
    j["verify_result"] = verify_result.toJson();
    j["error_message"] = error_message;
    return j;
  }
};

// Overall conformance test run result
struct N3ConformanceResult {
  size_t total_tests = 0;
  size_t passed_tests = 0;
  size_t failed_tests = 0;
  std::vector<N3TestResult> test_results;
  uint64_t total_runtime_ms = 0;

  nlohmann::json toJson() const {
    nlohmann::json j;
    j["total_tests"] = total_tests;
    j["passed_tests"] = passed_tests;
    j["failed_tests"] = failed_tests;
    j["total_runtime_ms"] = total_runtime_ms;

    nlohmann::json tests_json = nlohmann::json::array();
    for (const auto& test : test_results) {
      tests_json.push_back(test.toJson());
    }
    j["test_results"] = tests_json;

    return j;
  }

  std::string summary() const {
    return "Conformance tests: " + std::to_string(passed_tests) + "/" +
           std::to_string(total_tests) + " passed";
  }
};

// N3 conformance test runner
class N3ComplianceRunner {
 public:
  // Construct runner with configuration
  explicit N3ComplianceRunner(N3Config config = N3Config::defaultConfig());

  // Load test cases from JSON manifest file
  std::vector<N3TestCase> loadTestCases(const std::string& manifest_file) const;

  // Run all test cases from manifest
  N3ConformanceResult runConformanceTests(
      const std::string& manifest_file) const;

  // Run specific test cases
  N3ConformanceResult runTests(const std::vector<N3TestCase>& tests) const;

  // Run single test case
  N3TestResult runTest(const N3TestCase& test) const;

  // Write results to JSON file
  void writeResults(const N3ConformanceResult& results,
                    const std::string& output_file) const;

  // Compare results with expected results (for regression testing)
  bool compareResults(const N3ConformanceResult& actual,
                      const N3ConformanceResult& expected) const;

  // Get current configuration
  const N3Config& config() const { return config_; }

 private:
  N3Config config_;
  N3Service service_;
};

}  // namespace ad_engine::n3

#endif  // QLEVER_SRC_ENGINE_N3_N3COMPLIANCERUNNER_H
