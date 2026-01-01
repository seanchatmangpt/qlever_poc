//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#include "engine/conformance/ConformanceRunner.h"

#include <exception>

namespace ad_engine::conformance {

ConformanceRunnerResult ConformanceRunner::runAllTests(
    const std::string& manifestPath) {
  // Load test cases from manifest
  std::vector<ConformanceTestCase> testCases =
      ConformanceUtils::loadTestManifest(manifestPath);

  return runAllTests(testCases);
}

ConformanceRunnerResult ConformanceRunner::runAllTests(
    const std::vector<ConformanceTestCase>& testCases) {
  ConformanceRunnerResult result;
  result.runnerName = runnerName_;
  result.timestampIso8601 = getCurrentTimestampIso8601();
  result.summary = TestRunSummary();

  auto startTime = std::chrono::high_resolution_clock::now();

  // Run each test case
  for (const auto& testCase : testCases) {
    TestCaseResult testResult = runTest(testCase);
    result.testResults.push_back(testResult);
    updateSummary(result.summary, testResult);
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);
  result.summary.totalTimeMs = duration.count();

  return result;
}

TestCaseResult ConformanceRunner::runTest(
    const ConformanceTestCase& testCase) {
  TestCaseResult result;
  result.testName = testCase.testName;
  result.expectedOutcome = testCase.expectedOutcome;

  // Validate test case
  if (!validateTestCase(testCase, result)) {
    return result;
  }

  auto startTime = std::chrono::high_resolution_clock::now();

  try {
    // Execute the test
    nlohmann::json actualOutput = executeTest(testCase);
    result.actualData = actualOutput;

    // Load expected output (if available)
    nlohmann::json expectedOutput;
    if (!testCase.expectedOutputPath.empty()) {
      expectedOutput = loadExpectedOutput(testCase);
      result.expectedData = expectedOutput;
    }

    // Determine actual outcome
    if (actualOutput.contains("status")) {
      result.actualOutcome = actualOutput["status"].get<std::string>();
    } else if (actualOutput.contains("violations") &&
               actualOutput["violations"].is_array() &&
               !actualOutput["violations"].empty()) {
      result.actualOutcome = "VIOLATIONS";
      result.violationsActual = actualOutput["violations"].size();
    } else if (actualOutput.contains("error")) {
      result.actualOutcome = "ERROR";
    } else {
      result.actualOutcome = "PASS";
    }

    // Compare results if expected output is available
    if (!testCase.expectedOutputPath.empty()) {
      result.diff =
          compareResults(actualOutput, expectedOutput, testCase.testType);

      // Determine test status based on diff
      if (!result.diff.hasDifference) {
        result.status = TestStatus::Pass;
      } else {
        result.status = TestStatus::Fail;
      }

      // Extract expected metrics
      if (expectedOutput.contains("violations") &&
          expectedOutput["violations"].is_array()) {
        result.violationsExpected = expectedOutput["violations"].size();
      }
      if (expectedOutput.contains("fact_count")) {
        result.factsExpected = expectedOutput["fact_count"].get<size_t>();
      }
      if (actualOutput.contains("fact_count")) {
        result.factsActual = actualOutput["fact_count"].get<size_t>();
      }
    } else {
      // No expected output to compare against
      // Check if actual outcome matches expected outcome
      if (result.actualOutcome ==
          expectedOutcomeToString(testCase.expectedOutcome)) {
        result.status = TestStatus::Pass;
      } else {
        result.status = TestStatus::Fail;
      }
    }

  } catch (const std::exception& e) {
    result.status = TestStatus::Error;
    result.actualOutcome = "ERROR";
    result.errorMessage = e.what();
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
  result.timingMs = duration.count() / 1000.0;

  return result;
}

nlohmann::json ConformanceRunner::executeTest(
    const ConformanceTestCase& testCase) {
  if (!executor_) {
    throw std::runtime_error(
        "No test executor configured. Call setTestExecutor() or override "
        "executeTest().");
  }

  return executor_(testCase);
}

nlohmann::json ConformanceRunner::loadExpectedOutput(
    const ConformanceTestCase& testCase) {
  return ConformanceUtils::readJsonFile(testCase.expectedOutputPath);
}

DiffResult ConformanceRunner::compareResults(const nlohmann::json& actual,
                                             const nlohmann::json& expected,
                                             TestType testType) {
  // Use specialized differ based on test type
  switch (testType) {
    case TestType::Validation:
      return ConformanceDiffer::compareValidationResults(actual, expected);
    case TestType::Rules:
      return ConformanceDiffer::compareRuleResults(actual, expected);
    case TestType::N3Verify:
      return ConformanceDiffer::compareN3Results(actual, expected);
    case TestType::Performance:
      // For performance tests, use fuzzy matching with tolerance
      return ConformanceDiffer::compareJson(actual, expected, fuzzyConfig_);
    default:
      // Default: deep JSON comparison
      return ConformanceDiffer::compareJson(actual, expected, fuzzyConfig_);
  }
}

void ConformanceRunner::updateSummary(TestRunSummary& summary,
                                      const TestCaseResult& result) {
  summary.total++;

  switch (result.status) {
    case TestStatus::Pass:
      summary.passed++;
      break;
    case TestStatus::Fail:
      summary.failed++;
      break;
    case TestStatus::Skip:
      summary.skipped++;
      break;
    case TestStatus::Error:
      summary.errors++;
      break;
    case TestStatus::Timeout:
      summary.timeouts++;
      break;
  }
}

bool ConformanceRunner::validateTestCase(const ConformanceTestCase& testCase,
                                         TestCaseResult& result) {
  std::string errorMessage;
  if (!ConformanceUtils::validateTestCase(testCase, errorMessage)) {
    result.status = TestStatus::Error;
    result.actualOutcome = "ERROR";
    result.errorMessage = "Test case validation failed: " + errorMessage;
    return false;
  }

  // Check if test should be skipped
  if (testCase.expectedOutcome == ExpectedOutcome::Skip) {
    result.status = TestStatus::Skip;
    result.actualOutcome = "SKIP";
    return false;
  }

  return true;
}

}  // namespace ad_engine::conformance
