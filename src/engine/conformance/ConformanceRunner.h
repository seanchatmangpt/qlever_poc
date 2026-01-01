//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_RUNNER_H
#define QLEVER_CONFORMANCE_RUNNER_H

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "engine/conformance/ConformanceDiffer.h"
#include "engine/conformance/ConformanceResultSchema.h"
#include "engine/conformance/ConformanceTestCase.h"
#include "engine/conformance/ConformanceUtils.h"

namespace ad_engine::conformance {

// Test executor function signature
// Takes: test case, returns: actual output as JSON
using TestExecutor =
    std::function<nlohmann::json(const ConformanceTestCase&)>;

// Base runner class - subsystem-agnostic conformance test harness
class ConformanceRunner {
 public:
  // Constructor
  explicit ConformanceRunner(std::string runnerName)
      : runnerName_(std::move(runnerName)) {}

  // Virtual destructor for polymorphic use
  virtual ~ConformanceRunner() = default;

  // Set the test executor function
  void setTestExecutor(TestExecutor executor) { executor_ = std::move(executor); }

  // Set fuzzy match configuration
  void setFuzzyMatchConfig(FuzzyMatchConfig config) {
    fuzzyConfig_ = std::move(config);
  }

  // Run all tests from a manifest file
  ConformanceRunnerResult runAllTests(const std::string& manifestPath);

  // Run all tests from a vector of test cases
  ConformanceRunnerResult runAllTests(
      const std::vector<ConformanceTestCase>& testCases);

  // Run a single test case
  TestCaseResult runTest(const ConformanceTestCase& testCase);

  // Get runner name
  const std::string& getRunnerName() const { return runnerName_; }

 protected:
  // Subsystem-specific execution (to be overridden in derived classes)
  // Default implementation uses the executor_ function
  virtual nlohmann::json executeTest(const ConformanceTestCase& testCase);

  // Load expected output for a test case
  virtual nlohmann::json loadExpectedOutput(
      const ConformanceTestCase& testCase);

  // Compare actual vs expected using appropriate differ
  virtual DiffResult compareResults(const nlohmann::json& actual,
                                    const nlohmann::json& expected,
                                    TestType testType);

 private:
  std::string runnerName_;
  TestExecutor executor_;
  FuzzyMatchConfig fuzzyConfig_;

  // Helper: Update summary statistics
  void updateSummary(TestRunSummary& summary, const TestCaseResult& result);

  // Helper: Validate test case before running
  bool validateTestCase(const ConformanceTestCase& testCase,
                        TestCaseResult& result);
};

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_RUNNER_H
