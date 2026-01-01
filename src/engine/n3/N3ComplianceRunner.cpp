// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include "engine/n3/N3ComplianceRunner.h"

#include <chrono>
#include <fstream>
#include <iostream>

#include "util/Exception.h"
#include "util/Log.h"

namespace ad_engine::n3 {

// ____________________________________________________________________________
N3ComplianceRunner::N3ComplianceRunner(N3Config config)
    : config_(std::move(config)), service_(config_) {}

// ____________________________________________________________________________
std::vector<N3TestCase> N3ComplianceRunner::loadTestCases(
    const std::string& manifest_file) const {
  std::ifstream file(manifest_file);
  if (!file.is_open()) {
    AD_THROW("Failed to open manifest file: " + manifest_file);
  }

  nlohmann::json manifest_json;
  try {
    file >> manifest_json;
  } catch (const std::exception& e) {
    AD_THROW("Failed to parse manifest JSON: " + std::string(e.what()));
  }

  std::vector<N3TestCase> tests;
  if (manifest_json.contains("tests") && manifest_json["tests"].is_array()) {
    for (const auto& test_json : manifest_json["tests"]) {
      tests.push_back(N3TestCase::fromJson(test_json));
    }
  }

  return tests;
}

// ____________________________________________________________________________
N3ConformanceResult N3ComplianceRunner::runConformanceTests(
    const std::string& manifest_file) const {
  auto tests = loadTestCases(manifest_file);
  return runTests(tests);
}

// ____________________________________________________________________________
N3ConformanceResult N3ComplianceRunner::runTests(
    const std::vector<N3TestCase>& tests) const {
  auto start_time = std::chrono::steady_clock::now();

  N3ConformanceResult result;
  result.total_tests = tests.size();

  for (const auto& test : tests) {
    AD_LOG_INFO << "Running test: " << test.name << std::endl;
    auto test_result = runTest(test);

    if (test_result.passed) {
      result.passed_tests++;
    } else {
      result.failed_tests++;
    }

    result.test_results.push_back(std::move(test_result));
  }

  auto end_time = std::chrono::steady_clock::now();
  result.total_runtime_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                             start_time)
          .count();

  return result;
}

// ____________________________________________________________________________
N3TestResult N3ComplianceRunner::runTest(const N3TestCase& test) const {
  N3TestResult result;
  result.test_name = test.name;
  result.passed = false;

  // Run verification
  try {
    result.verify_result = service_.verifyFile(test.input_file);

    // Check if result matches expectation
    if (test.should_pass && result.verify_result.ok) {
      // Expected to pass and did pass
      result.passed = true;
    } else if (!test.should_pass && !result.verify_result.ok) {
      // Expected to fail and did fail
      result.passed = true;
    } else if (test.should_pass && !result.verify_result.ok) {
      // Expected to pass but failed
      result.error_message = "Expected test to pass but it failed with " +
                             std::to_string(result.verify_result.errors.size()) +
                             " error(s)";
    } else {
      // Expected to fail but passed
      result.error_message = "Expected test to fail but it passed";
    }
  } catch (const std::exception& e) {
    result.error_message = "Exception during test: " + std::string(e.what());
    result.verify_result.ok = false;
    result.verify_result.addError(
        ErrorRecord(N3ErrorCode::INTERNAL_ERROR, e.what()));
  }

  return result;
}

// ____________________________________________________________________________
void N3ComplianceRunner::writeResults(const N3ConformanceResult& results,
                                      const std::string& output_file) const {
  std::ofstream file(output_file);
  if (!file.is_open()) {
    AD_THROW("Failed to open output file: " + output_file);
  }

  nlohmann::json output = results.toJson();
  file << output.dump(2);  // Pretty print with 2 space indent

  AD_LOG_INFO << "Results written to: " << output_file << std::endl;
}

// ____________________________________________________________________________
bool N3ComplianceRunner::compareResults(
    const N3ConformanceResult& actual,
    const N3ConformanceResult& expected) const {
  // Compare total counts
  if (actual.total_tests != expected.total_tests ||
      actual.passed_tests != expected.passed_tests ||
      actual.failed_tests != expected.failed_tests) {
    return false;
  }

  // Compare individual test results
  if (actual.test_results.size() != expected.test_results.size()) {
    return false;
  }

  for (size_t i = 0; i < actual.test_results.size(); i++) {
    const auto& actual_test = actual.test_results[i];
    const auto& expected_test = expected.test_results[i];

    // Compare test names and pass/fail status
    if (actual_test.test_name != expected_test.test_name ||
        actual_test.passed != expected_test.passed) {
      return false;
    }

    // Compare verify result ok status
    if (actual_test.verify_result.ok != expected_test.verify_result.ok) {
      return false;
    }

    // Compare error counts (exact error messages may vary)
    if (actual_test.verify_result.errors.size() !=
        expected_test.verify_result.errors.size()) {
      return false;
    }

    // Compare compliance digests (should be deterministic)
    if (actual_test.verify_result.ok &&
        actual_test.verify_result.compliance_digest !=
            expected_test.verify_result.compliance_digest) {
      return false;
    }
  }

  return true;
}

}  // namespace ad_engine::n3
