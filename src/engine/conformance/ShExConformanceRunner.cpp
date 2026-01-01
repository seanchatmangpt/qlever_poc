// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "util/Exception.h"
#include "util/Log.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

// Structure to represent a test case
struct ShExTestCase {
  std::string testDir;
  std::string testName;
  std::string description;
  std::string startShape;
  int expectedViolationsCount;
  std::vector<std::string> expectedErrorCodes;
  bool guardsTriggered;
  std::vector<std::string> featureCoverage;
  bool expectsRejection = false;
  json validationConfig;

  // File paths
  std::string inputTtlPath;
  std::string shapesShexPath;
  std::string expectedJsonPath;
  std::string metadataJsonPath;
};

// Structure to represent test results
struct TestResult {
  std::string testName;
  bool passed;
  std::string message;
  json actualResult;
  json expectedResult;
};

// Load metadata for a test case
ShExTestCase loadTestCase(const std::string& testDir) {
  ShExTestCase testCase;
  testCase.testDir = testDir;

  // Load metadata.json
  std::string metadataPath = testDir + "/metadata.json";
  std::ifstream metadataFile(metadataPath);
  if (!metadataFile.is_open()) {
    throw std::runtime_error("Failed to open metadata file: " + metadataPath);
  }

  json metadata;
  metadataFile >> metadata;

  testCase.testName = metadata["test_name"];
  testCase.description = metadata["description"];
  testCase.startShape = metadata["start_shape"];
  testCase.expectedViolationsCount = metadata["expected_violations_count"];
  testCase.guardsTriggered = metadata["guards_triggered"];

  if (metadata.contains("expected_error_codes")) {
    testCase.expectedErrorCodes =
        metadata["expected_error_codes"].get<std::vector<std::string>>();
  }

  if (metadata.contains("feature_coverage")) {
    testCase.featureCoverage =
        metadata["feature_coverage"].get<std::vector<std::string>>();
  }

  if (metadata.contains("expects_rejection")) {
    testCase.expectsRejection = metadata["expects_rejection"];
  }

  if (metadata.contains("validation_config")) {
    testCase.validationConfig = metadata["validation_config"];
  }

  // Set file paths
  testCase.inputTtlPath = testDir + "/input.ttl";
  testCase.shapesShexPath = testDir + "/shapes.shex";
  testCase.expectedJsonPath = testDir + "/expected.json";
  testCase.metadataJsonPath = metadataPath;

  return testCase;
}

// Load expected result from expected.json
json loadExpectedResult(const std::string& expectedJsonPath) {
  std::ifstream expectedFile(expectedJsonPath);
  if (!expectedFile.is_open()) {
    throw std::runtime_error("Failed to open expected result file: " +
                             expectedJsonPath);
  }

  json expected;
  expectedFile >> expected;
  return expected;
}

// Run validation for a test case
// NOTE: This is a stub that would call the actual ShEx validation service
json runShExValidation(const ShExTestCase& testCase) {
  // TODO: Implement actual ShEx validation by calling ValidationService
  // This would:
  // 1. Load the RDF data from input.ttl
  // 2. Parse the ShEx shapes from shapes.shex
  // 3. Run validation with the specified start shape
  // 4. Return the validation result as JSON

  // For now, return a placeholder result
  json result;
  result["conforms"] = true;
  result["violations"] = json::array();
  result["stats"]["focus_nodes_checked"] = 0;
  result["stats"]["total_violations"] = 0;
  result["stats"]["guards_triggered"] = json::array();

  // Add a note that this is a placeholder
  result["__note"] =
      "Placeholder result - actual validation not yet implemented";

  return result;
}

// Compare actual and expected results
TestResult compareResults(const ShExTestCase& testCase, const json& actual,
                          const json& expected) {
  TestResult result;
  result.testName = testCase.testName;
  result.actualResult = actual;
  result.expectedResult = expected;

  // Check if both results conform in the same way
  if (actual["conforms"] != expected["conforms"]) {
    result.passed = false;
    result.message = "Conformance mismatch: expected " +
                     expected["conforms"].dump() + ", got " +
                     actual["conforms"].dump();
    return result;
  }

  // Check violation count
  if (actual.contains("violations") && expected.contains("violations")) {
    size_t actualCount = actual["violations"].size();
    size_t expectedCount = expected["violations"].size();
    if (actualCount != expectedCount) {
      result.passed = false;
      result.message = "Violation count mismatch: expected " +
                       std::to_string(expectedCount) + ", got " +
                       std::to_string(actualCount);
      return result;
    }
  }

  // Check for error field (for unsupported features)
  if (expected.contains("error")) {
    if (!actual.contains("error")) {
      result.passed = false;
      result.message = "Expected error for unsupported feature, but got none";
      return result;
    }

    if (actual["error"]["code"] != expected["error"]["code"]) {
      result.passed = false;
      result.message = "Error code mismatch: expected " +
                       expected["error"]["code"].dump() + ", got " +
                       actual["error"]["code"].dump();
      return result;
    }
  }

  // Check guards triggered
  if (expected.contains("stats") && expected["stats"].contains("guards_triggered")) {
    if (!actual.contains("stats") || !actual["stats"].contains("guards_triggered")) {
      result.passed = false;
      result.message = "Missing guards_triggered in actual result";
      return result;
    }

    auto expectedGuards = expected["stats"]["guards_triggered"];
    auto actualGuards = actual["stats"]["guards_triggered"];
    if (expectedGuards != actualGuards) {
      result.passed = false;
      result.message = "Guards triggered mismatch";
      return result;
    }
  }

  result.passed = true;
  result.message = "Test passed";
  return result;
}

// Discover all test cases in the conformance/shex directory
std::vector<std::string> discoverTestCases(const std::string& baseDir) {
  std::vector<std::string> testDirs;

  if (!fs::exists(baseDir)) {
    throw std::runtime_error("Conformance directory not found: " + baseDir);
  }

  for (const auto& entry : fs::directory_iterator(baseDir)) {
    if (entry.is_directory()) {
      std::string dirName = entry.path().filename().string();
      if (dirName.substr(0, 5) == "test_") {
        testDirs.push_back(entry.path().string());
      }
    }
  }

  // Sort test directories for consistent ordering
  std::sort(testDirs.begin(), testDirs.end());

  return testDirs;
}

// Generate summary report
json generateSummary(const std::vector<TestResult>& results) {
  json summary;
  int passedCount = 0;
  int failedCount = 0;

  json testResults = json::array();
  for (const auto& result : results) {
    json testJson;
    testJson["test_name"] = result.testName;
    testJson["passed"] = result.passed;
    testJson["message"] = result.message;

    if (!result.passed) {
      testJson["expected"] = result.expectedResult;
      testJson["actual"] = result.actualResult;
    }

    testResults.push_back(testJson);

    if (result.passed) {
      passedCount++;
    } else {
      failedCount++;
    }
  }

  summary["total_tests"] = results.size();
  summary["passed"] = passedCount;
  summary["failed"] = failedCount;
  summary["pass_rate"] =
      results.empty() ? 0.0
                      : (static_cast<double>(passedCount) / results.size());
  summary["test_results"] = testResults;

  return summary;
}

int main(int argc, char** argv) {
  try {
    // Determine conformance directory
    std::string baseDir = "conformance/shex";
    if (argc > 1) {
      baseDir = argv[1];
    }

    LOG(INFO) << "ShEx Conformance Runner" << std::endl;
    LOG(INFO) << "Conformance directory: " << baseDir << std::endl;

    // Discover test cases
    auto testDirs = discoverTestCases(baseDir);
    LOG(INFO) << "Found " << testDirs.size() << " test cases" << std::endl;

    if (testDirs.empty()) {
      LOG(WARN) << "No test cases found in " << baseDir << std::endl;
      return 1;
    }

    // Run all test cases
    std::vector<TestResult> results;
    for (const auto& testDir : testDirs) {
      try {
        LOG(INFO) << "Running test: " << testDir << std::endl;

        // Load test case
        auto testCase = loadTestCase(testDir);
        LOG(INFO) << "  Test name: " << testCase.testName << std::endl;
        LOG(INFO) << "  Description: " << testCase.description << std::endl;

        // Load expected result
        auto expected = loadExpectedResult(testCase.expectedJsonPath);

        // Run validation
        auto actual = runShExValidation(testCase);

        // Compare results
        auto result = compareResults(testCase, actual, expected);
        results.push_back(result);

        LOG(INFO) << "  Result: " << (result.passed ? "PASS" : "FAIL")
                  << std::endl;
        if (!result.passed) {
          LOG(INFO) << "  Message: " << result.message << std::endl;
        }

      } catch (const std::exception& e) {
        LOG(ERROR) << "Error running test " << testDir << ": " << e.what()
                   << std::endl;
        TestResult errorResult;
        errorResult.testName = testDir;
        errorResult.passed = false;
        errorResult.message = std::string("Exception: ") + e.what();
        results.push_back(errorResult);
      }
    }

    // Generate summary
    auto summary = generateSummary(results);

    // Output summary to file
    std::string outputPath = baseDir + "/runner_output.json";
    std::ofstream outputFile(outputPath);
    if (outputFile.is_open()) {
      outputFile << summary.dump(2);
      outputFile.close();
      LOG(INFO) << "Summary written to: " << outputPath << std::endl;
    } else {
      LOG(ERROR) << "Failed to write summary to: " << outputPath << std::endl;
    }

    // Print summary to console
    std::cout << "\n=== ShEx Conformance Test Summary ===" << std::endl;
    std::cout << "Total tests: " << summary["total_tests"] << std::endl;
    std::cout << "Passed: " << summary["passed"] << std::endl;
    std::cout << "Failed: " << summary["failed"] << std::endl;
    std::cout << "Pass rate: " << (summary["pass_rate"].get<double>() * 100)
              << "%" << std::endl;

    // Return non-zero exit code if any tests failed
    return summary["failed"].get<int>() > 0 ? 1 : 0;

  } catch (const std::exception& e) {
    LOG(ERROR) << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
